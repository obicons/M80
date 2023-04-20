// Based on the code from V8's embedding example.

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include "include/libplatform/libplatform.h"
#include "include/v8.h"
#include "tcp_socket.hh"
#include "nacl_loader.hh"

extern "C" {
#include <dlfcn.h>
}

// This should never need used.
std::unique_ptr<v8::Platform> platform;

// Relates page names to JavaScript functions to produce their body.
// Readonly after initialization.
std::map<std::string, std::string> page_to_js_function;

// Relates page names to dynamic libraries to produce their body.
// Readonly after initialization.
std::map<std::string, void*> page_to_dl_handle;

// Relates page names to nacl contexts.
// Readonly after initialization.
std::map<std::string, std::unique_ptr<NaClContext>> page_to_nacl_context;

// Initializes V8.
static void initialize_v8(const char *location);

// Initializes JS resources.
static void initialize_resources();

// Returns the resource being accessed in the request.
static std::string get_resource(const std::string &request);

// Handles a HTTP request.
static void handle_request(TCPSocket client);

// Handles a HTTP request for a JS resource.
static void handle_js_request(TCPSocket client, const std::string &resource);

static void handle_dl_request(TCPSocket client, const std::string &resource);

int main(int argc, char* argv[]) {
  initialize_v8(argv[0]);
  initialize_resources();

  page_to_nacl_context["a.out"] = NaClContext::create_context("native_client_bin/a.out");
  if (page_to_nacl_context["a.out"] == nullptr) {
    std::cerr << "Could not allocate native client sandbox." << std::endl;
    return 1;
  }

  std::cout << "Created sandbox." << std::endl;
  std::cout << "Sandbox output: " << page_to_nacl_context["a.out"]->call().value() << std::endl;
  std::cout << "This verifies the sandbox is provisioned and can execute client code." << std::endl;
  

  std::optional<TCPSocket> socket = TCPSocket::open("0.0.0.0", 8080);
  if (!socket.has_value()) {
    std::cerr << "Could not open socket: " << strerror(errno) << std::endl;
    return 1;
  }

  while (true) {
    std::optional<TCPSocket> client = socket.value().accept();
    if (!client.has_value()) {
      std::cerr << "Could not accept(): " << strerror(errno) << std::endl;
      return 1;
    }

    handle_request(client.value());
  }
}

static void initialize_v8(const char *location) {
  v8::V8::InitializeICUDefaultLocation(location);
  v8::V8::InitializeExternalStartupData(location);
  platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();
}

// Initializes all resources.
static void initialize_resources() {
  for (const auto &entry : std::filesystem::directory_iterator("resources/")) {
    if (entry.is_regular_file()) {
      std::ifstream file(entry.path());
      std::stringstream file_contents;
      file_contents << file.rdbuf();
      page_to_js_function[entry.path().filename()] = file_contents.str();
    }
  }

  for (const auto &entry : std::filesystem::directory_iterator("build/lib/")) {
    void *handle = dlopen(entry.path().c_str(), RTLD_NOW);
    if (!handle) {
      std::cerr << "Unable to dlopen(): " << strerror(errno) << std::endl;
      std::exit(1);
    }

    page_to_dl_handle[entry.path().filename()] = handle;
  }
}

static std::string get_resource(const std::string &request) {
  if (request.length() == 0) {
    return "";
  }

  size_t start = 0;
  while (start < request.length() - 1 && !isspace(request[start])) {
    start++;
  }
  start++;


  size_t count = 1;
  while (start + count < request.length() && !isspace(request[start + count])) {
    count++;
  }

  std::string request_str = request.substr(start, count);
  if (request_str.starts_with("/")) {
    return request_str.substr(1);
  }

  return request_str;
}

static void handle_sandbox_request(TCPSocket client, const std::string &resource) {
  std::unique_ptr<NaClContext> &sandbox = page_to_nacl_context[resource];
  std::optional<std::string> result = sandbox->call();
  if (!result.has_value()) {
    std::cout << "PROBLEM" << std::endl;
    client.write("HTTP/1.1 500 Internal Server Error");
  } else {
    client.write("HTTP/1.1 200 OK\r\n\r\n" + result.value());
  }
}

static void handle_request(TCPSocket client) {
  std::array<char, 1024> buffer;
  ssize_t nread = read(client, buffer.data(), buffer.size());
  std::string request_string(buffer.begin(), buffer.begin() + nread);
  std::string resource = get_resource(request_string);

  if (page_to_js_function.contains(resource)) {
    handle_js_request(client, resource);
  } else if (page_to_dl_handle.contains(resource)) {
    handle_dl_request(client, resource);
  } else if (resource == "a.out") {
    handle_sandbox_request(client, "a.out");
  } else {
    client.write("HTTP/1.1 404 Not Found\r\n\r\nnot found");
  }
}

// Handles a HTTP request for a JS resource.
static void handle_js_request(TCPSocket client, const std::string &resource) {
  // Create a new Isolate and make it the current one.
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
    v8::ArrayBuffer::Allocator::NewDefaultAllocator();

  v8::Isolate* isolate = v8::Isolate::New(create_params);
  {
    v8::Isolate::Scope isolate_scope(isolate);

    // Create a stack-allocated handle scope.
    v8::HandleScope handle_scope(isolate);

    // Create a new context.
    v8::Local<v8::Context> context = v8::Context::New(isolate);

    // Enter the context for compiling.
    v8::Context::Scope context_scope(context);

    // Create a string containing the JavaScript source code.
    v8::Local<v8::String> source =
      v8::String::NewFromUtf8(isolate, page_to_js_function[resource].c_str(),
                              v8::NewStringType::kNormal).ToLocalChecked();

    // Compile the source code.
    v8::Local<v8::Script> script =
      v8::Script::Compile(context, source).ToLocalChecked();
    script->Run(context).ToLocalChecked();

    v8::MaybeLocal<v8::Value> maybe_main_func =
      context->Global()->Get(context, v8::String::NewFromUtf8(isolate, "main")
                         .ToLocalChecked());

    if (maybe_main_func.IsEmpty()) {
      client.write("HTTP/1.1 500 Internal Server Error\r\n");
      return;
    }

    v8::Local<v8::Value> main_func = maybe_main_func.ToLocalChecked();
    if (!main_func->IsFunction()) {
      client.write("HTTP/1.1 500 Internal Server Error\r\n");
      return;
    }

    v8::MaybeLocal<v8::Value> maybe_return_value =
        main_func.As<v8::Function>()->Call(context, main_func, 0, nullptr);
    if (maybe_return_value.IsEmpty()) {
      client.write("HTTP/1.1 500 Internal Server Error\r\n");
      return;
    }

    v8::Local<v8::Value> rvalue = maybe_return_value.ToLocalChecked();
    if (!rvalue->IsString()) {
      client.write("HTTP/1.1 500 Internal Server Error\r\n");
      return;
    }

    client.write("HTTP/1.1 200 OK\r\n\r\n" + std::string(*v8::String::Utf8Value(isolate, rvalue)));
  }

  // Dispose the isolate and tear down V8.
  isolate->Dispose();
  delete create_params.array_buffer_allocator;
}

static void handle_dl_request(TCPSocket client, const std::string &resource) {
  void *dl_handle = page_to_dl_handle[resource];

  const char* (*http_main)(void) =
    (const char* (*)(void)) dlsym(dl_handle, "http_main");

  if (!http_main) {
    client.write("HTTP/1.1 500 Internal Server Error\r\n");
    return;
  }

  std::string response = http_main();
  client.write("HTTP/1.1 200 OK\r\n\r\n" + response);
}