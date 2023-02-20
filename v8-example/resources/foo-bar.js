function fib(num) {
    if (num < 2) {
        return 1;
    }
    return fib(num - 2) + fib(num - 1);
}

function main() {
    let result = fib(10);
    return result.toString();
}