// Fib : Calculate fibonacci (30)
//
// Compile:
//   $ ./xcc -ofib examples/fib.c
//
// Run:
//   $ ./fib  #=> 832040

int fib(int n) {
    if (n < 2)
        return n;
    else
        return fib(n - 1) + fib(n - 2);
}

int main() {
    return fib(23);
}
