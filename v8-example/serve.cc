// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include "include/libplatform/libplatform.h"
#include "include/v8.h"

// This should never need used.
std::unique_ptr<v8::Platform> platform;

// Relates page names to JavaScript functions to produce their body.
// Readonly after initialization.
std::map<std::string, std::string> page_to_js_function;

// Initializes V8.
static void initialize_v8(const char *location);

// Initializes JS resources.
static void initialize_resources();

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " [JS resource to invoke]" << std::endl;
    return 1;
  }

  const std::string js_resource = argv[1];
  initialize_v8(argv[0]);
  initialize_resources();

  if (!page_to_js_function.contains(js_resource)) {
    std::cerr << "unrecognized JS resource: " << js_resource << std::endl;
    return 1;
  }

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
      v8::String::NewFromUtf8(isolate, page_to_js_function[js_resource].c_str(),
                              v8::NewStringType::kNormal).ToLocalChecked();

    // Compile the source code.
    v8::Local<v8::Script> script =
      v8::Script::Compile(context, source).ToLocalChecked();
    script->Run(context).ToLocalChecked();

    v8::MaybeLocal<v8::Value> maybe_main_func =
      context->Global()->Get(context, v8::String::NewFromUtf8(isolate, "main")
                         .ToLocalChecked());

    if (maybe_main_func.IsEmpty()) {
      std::cerr << "error: JS resource does not define a main() function." << std::endl;
      return 1;
    }

    v8::Local<v8::Value> main_func = maybe_main_func.ToLocalChecked();
    if (!main_func->IsFunction()) {
      std::cerr << "error: JS resource does not define a main() function." << std::endl;
      return 1;
    }

    v8::MaybeLocal<v8::Value> maybe_return_value =
        main_func.As<v8::Function>()->Call(context, main_func, 0, nullptr);
    if (maybe_return_value.IsEmpty()) {
      std::cerr << "error: JS resource did not return a value." << std::endl;
      return 1;
    }

    v8::Local<v8::Value> rvalue = maybe_return_value.ToLocalChecked();
    if (!rvalue->IsString()) {
      std::cerr << "error: JS resource did not return a string." << std::endl;
      return 1;
    }

    std::cout << *v8::String::Utf8Value(isolate, rvalue) << std::endl;
  }

  // Dispose the isolate and tear down V8.
  isolate->Dispose();
  v8::V8::Dispose();
  delete create_params.array_buffer_allocator;
}

static void initialize_v8(const char *location) {
  v8::V8::InitializeICUDefaultLocation(location);
  v8::V8::InitializeExternalStartupData(location);
  platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();
}

// Initializes JS resources.
static void initialize_resources() {
  for (const auto &entry : std::filesystem::directory_iterator("resources/")) {
    if (entry.is_regular_file()) {
      std::ifstream file(entry.path());
      std::stringstream file_contents;
      file_contents << file.rdbuf();
      page_to_js_function[entry.path().filename()] = file_contents.str();
    }
  }
}