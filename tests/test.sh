#!/bin/bash

XCC=../xcc

try_direct() {
  title="$1"
  expected="$2"
  input="$3"

  echo -n "$title => "

  echo -e "$input" | $XCC || exit 1

  ./a.out
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "OK"
  else
    echo "NG: $expected expected, but got $actual"
    exit 1
  fi
}

try() {
  try_direct "$1" "$2" "int main(void){$3}"
}

try_output_direct() {
  title="$1"
  expected="$2"
  input="$3"

  echo -n "$title => "

  echo -e "$input" | $XCC || exit 1

  actual=`./a.out` || exit 1

  if [ "$actual" = "$expected" ]; then
    echo "OK"
  else
    echo "NG: $expected expected, but got $actual"
    exit 1
  fi
}

try_output() {
  try_output_direct "$1" "$2" "int main(){ $3 return 0; }"
}

compile_error() {
  title="$1"
  input="$2"

  echo -n "$title => "

  echo -e "$input" | $XCC
  result="$?"

  if [ "$result" = "0" ]; then
    echo "NG: Compile error expected, but succeeded"
    exit 1
  fi
}

try_output 'write' 'hello' "_write(1, \"hello\\\\n\", 6);"
try_output 'char array' 123 "char s[16]; s[0] = '1'; s[1] = '2'; s[2] = '3'; s[3] = '\\\\n'; _write(1, s, 4);"
try_output 'string initializer' 'aBc' "char s[] = \"abc\\\\n\"; s[1] = 'B'; _write(1, s, 4);"
try_direct 'enum' 11 'enum Num { Zero, One, Two }; int main(){ return One + 10; }'
try_direct 'enum with assign' 11 'enum Num { Ten = 10, Eleven }; int main(){ return Eleven; }'
try_direct 'enum can use in case' 1 'enum Num { Zero, One, Two }; int main(){ switch (1) { case One: return 1; } return 0; }'
try_direct 'typedef' 123 'typedef struct {int x;} Foo; int main(){ Foo foo; foo.x = 123; return foo.x; }'

# error cases
echo ''
echo '### Error cases'
compile_error 'no main' 'void foo(){}'
compile_error 'undef var' 'void main(){ x = 1; }'
compile_error 'undef funcall' 'void foo(); void main(){ foo(); }'
compile_error 'no proto def' 'void main(){ foo(); } void foo(){}'
compile_error 'int - ptr' 'void main(){ int *p; p = 1; 2 - p; }'
compile_error '*num' 'void main(){ *123; }'
compile_error '&num' 'void main(){ &123; }'
compile_error '&enum' 'enum Num { Zero }; void main(){ void *p = &Zero; }'
compile_error 'assign to non-lhs' 'void main(){ int x; x + 1 = 3; }'
compile_error '+= to non-lhs' 'void main(){ int x; x + 1 += 3; }'
compile_error 'implicit cast to ptr' 'void foo(int *p); void main(){ foo(123); }'
compile_error 'struct->' 'struct Foo{int x;}; void main(){ struct Foo foo; foo->x; }'
compile_error 'struct*.' 'struct Foo{int x;}; void main(){ struct Foo* p; p.x; }'
compile_error 'int*->' 'void main(){ int *p; p->x; }'
compile_error 'void var' 'void main(){ void x; }'
compile_error 'void param' 'void main(void x){}'
compile_error 'void expr' 'void main(){ 1 + (void)2; }'
compile_error 'few arg num' 'void foo(int x); void main(){ foo(); }'
compile_error 'many arg num' 'void foo(int x); void main(){ foo(1, 2); }'
compile_error '+ str' 'void main(){ +"foo"; }'
compile_error '- str' 'void main(){ -"foo"; }'
compile_error 'break outside loop' 'void main(){ break; }'
compile_error 'continue outside loop' 'void main(){ continue; }'
compile_error 'return void' 'void main(){ return 1; }'
compile_error 'return non-void' 'int main(){ return; }'
compile_error 'use before decl' 'void main(){ x = 0; int x; }'
compile_error 'scope invisible' 'int main(){ {int x;} return x; }'
compile_error 'array = ptr' 'void main(){ int a[1], *p; a = p; }'
compile_error 'case outside switch' 'void main(){ switch(0){} case 0: return 0; }'
compile_error 'default outside switch' 'void main(){ switch(0){} default: return 0; }'
compile_error 'vardecl is not stmt' 'int main(){ if (1) int x = 0; return x; }'
compile_error 'static cannot use for local yet' 'void main(){ static int x = 123; }'