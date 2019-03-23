// #include "util.c"

void expect(char *title, long expected, long actual) {
  puts(title);
  puts(" => ");
  if (expected == actual) {
    puts("OK\n");
    return;
  }
  puts("NG: ");
  putdeci(expected);
  puts(" expected, but got ");
  putdeci(actual);
  puts("\n");
  exit(1);
}

int g_zero, g_work, g_123 = 123;

struct {int x; int *p;} g_struct = { 42, &g_zero };

static int s_val = 456;

extern int e_val;

int protodecl(int);

int foo() {
  return 123;
}

int sqsub(int x, int y) {
  int xx = x * x; int yy = y * y;
  return xx - yy;
}

int sub(int x, int y) {
  return x - y;
}

int apply(int (*f)(int, int), int x, int y) {
  return f(x, y);
}

int array_from_ptr1(int a[]) {
  return a[0];
}
int array_from_ptr2(int a[][2]) {
  return a[1][1];
}
int array_from_ptr3(int a[][3]) {
  return a[1][1];
}
int ptr_from_array(int *p) {
  return *(p + 1);
}

int main(void) {
  expect("zero", 0, 0);
  expect("decimal", 42, 42);
  expect("hex", 18, 0x12);
  expect("octal", 83, 0123);
  expect("negative", -42, -42);
  expect("long", 123, 123L);
  expect("+-", 21, 5+20-4);
  expect("token", 41,  12 + 34 - 5 );
  expect("*+", 47, 5+6*7);
  expect("()", 15, 5*(9-6));
  expect("/", 4, (3+5)/2);
  expect("%", 3, 123%10);
  expect("<<", 32, 1 << 4 + 1);
  expect(">>", 0x0a, 0xa5 >> 4);
  expect("&", 0xa0, 0xa5 & 0xf0);
  expect("|", 0x66, 0xc3 ^ 0xa5);
  {
    short x = 3;
    expect("short", 1, x == 3 && sizeof(x) == 2);
  }
  {
    long x = 3;
    expect("long arithmetic", 3, 5L + 4L - x * 2L / 1L);
  }
  expect("unsigned char", -128, (unsigned char)0x80);  // TODO: Fix
  {
    int a = 3;
    int b = 5 * 6 - 8;
    expect("variable", 14, a + b / 2);
  }
  {
    int foo = 3;
    int bar = 5 * 6 - 8;
    expect("variable2", 14, foo + bar / 2);
  }
  {
    int x = 42;
    expect("positive var", 42, +x);
  }
  {
    int x = 42;
    expect("negative var", -42, -x);
  }
  {
    int a, b, c;
    a = b = (c = 1) + 2;
    expect("assign", 1, a == b);
  }
  expect("!=", 1, 123 != 456);
  expect("not true", 0, !(1 == 1));
  expect("not false", 1, !(0 == 1));
  {
    int x = 1;
    int y = ++x;
    expect("preinc", 4, x + y);
  }
  {
    int x = 1;
    int y = --x;
    expect("preinc", 0, x + y);
  }
  {
    int x = 1;
    int y = x++;
    expect("postinc", 3, x + y);
  }
  {
    int x = 1;
    int y = x--;
    expect("postinc", 1, x + y);
  }
  {
    int x = 10;
    x += 3;
    expect("+=", 13, x);
  }
  {
    int x = 10;
    x -= 3;
    expect("-=", 7, x);
  }
  {
    int x = 10;
    x *= 3;
    expect("*=", 30, x);
  }
  {
    int x = 10;
    x /= 3;
    expect("/=", 3, x);
  }
  {
    int x = 10;
    x %= 3;
    expect("%=", 1, x);
  }
  expect("funcall", 23, foo() - 100);
  expect("func var", 9, sqsub(5, 4));
  {
    int x = 0;
    if (1)
      x = 2;
    expect("if", 2, x);
  }
  {
    int x = 0;
    if (0)
      x = 2;
    expect("if-false", 0, x);
  }
  {
    int x = 0;
    if (1 == 1)
      x = 2;
    else
      x = 3;
    expect("if else", 2, x);
  }
  {
    int x = 0;
    if (1 == 0)
      x = 2;
    else
      x = 3;
    expect("if else-false", 3, x);
  }
  {
    int a = 0, b = 0;
    if (1) {
      a = 1;
      b = 2;
    }
    expect("block statement", 3, a + b);
  }
  {
    int x = 0;
    if (1)
      ;
    else
      x = 1;
    expect("empty statement", 0, x);
  }
  {
    int i = 0, acc = 0;
    while (i <= 10) {
      acc += i;
      ++i;
    }
    expect("while", 55, acc);
  }
  {
    int i, acc;
    for (i = acc = 0; i <= 10; i++)
      acc += i;
    expect("for", 55, acc);
  }
  {
    int x = 0;
    switch (1) {
    case 1:
      x = 11;
      break;
    default:
      x = 22;
      break;
    }
    expect("switch", 11, x);
  }
  {
    int x = 0;
    switch (2) {
    case 1:
      x = 11;
      break;
    default:
      x = 22;
      break;
    }
    expect("switch default", 22, x);
  }
  {
    int x = 0;
    switch (3) {
    case 1:
      x = 11;
      break;
    }
    expect("switch no-default", 0, x);
  }
  {
    int x = 0;
    switch (1) {
    case 1:
      x += 1;
      // Fallthrough
    default:
      x += 10;
    }
    expect("switch fallthrough", 11, x);
  }
  {
    int x = 10, *p = &x;
    ++(*p);
    expect("pointer", 11, x);
  }
  {
    int a[3], *p = a;
    ++p;
    *p = 123;
    expect("array", 123, *(a + 1));
  }
  {
    int a[2];
    *a = 1;
    a[1] = 10;
    expect("array access", 11, a[0] + 1[a]);
  }
  {
    int a[2], *p;
    a[0] = 10;
    a[1] = 20;
    p = a;
    expect("preinc pointer", 20, *(++p));
  }
  {
    int a[2], *p;
    a[0] = 10;
    a[1] = 20;
    p = a;
    expect("postinc pointer", 10, *p++);
  }
  {
    int a[2], *p;
    a[0] = 98;
    a[1] = 76;
    p = a;
    p += 1;
    expect("pointer +=", 76, *p);
  }
  {
    int a[2], *p;
    a[0] = 54;
    a[1] = 32;
    p = &a[1];
    p -= 1;
    expect("pointer +=", 54, *p);
  }
  {
    int a[2], *p;
    a[0] = 11;
    a[1] = 22;
    p = a;
    *(p++) += 100;
    expect("postinc +=, 1", 111, a[0]);
    expect("postinc +=, 2", 22, a[1]);
  }
  {
    int x = 0;
    expect("cast pointer", 0, *(int*)&x);
  }
  {
    char x, *p = &x;
    expect("cast pointer", 1, (void*)&x == (void(*)())p);
  }
  {
    void *p = (void*)1234;
    expect("cast pointer", 1234L, (long)p);
  }
  expect("global cleared", 0, g_zero);
  expect("global initializer", 123, g_123);
  expect("global struct initializer: int", 42, g_struct.x);
  expect("global struct initializer: ptr", (long)&g_zero, (long)g_struct.p);
  {
    g_work = 1;
    expect("global access", 11, g_work + 10);
  }
  {
    struct {char x; int y;} foo;
    foo.x = 1;
    foo.y = 2;
    expect("struct", 3, foo.x + foo.y);
  }
  {
    struct {char x; int y;} foo, *p = &foo;
    p->x = 1;
    p->y = 2;
    expect("struct pointer", 3, foo.x + foo.y);
  }
  {
    union {char x; int y;} foo;
    expect("union", 1, sizeof(foo) == sizeof(int) && (void*)&foo.x == (void*)&foo.y);
  }
  {
    struct{
      union{
        int x;
      };
    } a;
    a.x = 596;
    expect("anonymous", 596, a.x);
    expect("anonymous adr", (long)&a, (long)&a.x);
  }
  expect("func pointer", 9, apply(&sub, 15, 6));
  expect("block comment", 123, /* comment */ 123);
  expect("line comment", 123, // comment
         123);
  expect("proto decl", 12321, protodecl(111));
  {
    int i, acc;
    for (i = acc = 0; i <= 10; i++) {
      if (i == 5)
        break;
      acc += i;
    }
    expect("for-break", 10, acc);
  }
  {
    int i, acc;
    for (i = acc = 0; i <= 10; i++) {
      if (i == 5)
        continue;
      acc += i;
    }
    expect("for-continue", 50, acc);
  }
  {
    int i = 0, acc = 0;
    while (++i <= 10) {
      if (i == 5)
        break;
      acc += i;
    }
    expect("while-break", 10, acc);
  }
  {
    int i = 0, acc = 0;
    while (++i <= 10) {
      if (i == 5)
        continue;
      acc += i;
    }
    expect("while-continue", 50, acc);
  }
  {
    int i = 0, acc = 0;
    do {
      if (i == 5)
        break;
      acc += i;
    } while (++i <= 10);
    expect("do-while-break", 10, acc);
  }
  {
    int i = 0, acc = 0;
    do {
      if (i == 5)
        continue;
      acc += i;
    } while (++i <= 10);
    expect("do-while-continue", 50, acc);
  }
  expect("t && t", 1, 1 && 2);
  {
    int x = 1;
    0 && (x == 0);
    expect("&& shortcut", 1, x);
  }
  expect("f || t", 1, 0 || 2);
  {
    int x = 1;
    1 || (x == 0);
    expect("|| shortcut", 1, x);
  }
  {
    int x = 1;
    { int x = 2; }
    expect("block scope", 1, x);
  }
  {
    char a[2][3];
    a[1][0] = 1;
    expect("nested-array", 1, ((char*)a)[3]);
  }
  {
    int a[2];
    a[1] = 45;
    expect("array <- ptr", 45, array_from_ptr1(&a[1]));
  }
  {
    int a[3][2];
    a[1][1] = 39;
    expect("array <- ptr:2", 39, array_from_ptr2(a));
  }
  {
    int a[3][2];
    a[2][0] = 987;
    expect("array <- ptr:3", 987, array_from_ptr3((int[][3])a));
  }
  {
    int a[2];
    a[1] = 55;
    expect("ptr <- array", 55, ptr_from_array(a));
  }
  expect("sizeof(int)", 4, sizeof(int));
  expect("sizeof(long)", 8, sizeof(long));
  expect("sizeof(array)", 3, sizeof(char [3]));
  {
    int a[5];
    expect("sizeof(array len)", 5, sizeof(a) / sizeof(*a));
  }
  expect("sizeof(struct)", 8, sizeof(struct {int a; char b;}));
  expect("sizeof(empty struct)", 0, sizeof(struct {}));
  {
    struct {} a, b;
    expect("empty struct occupy", 1, &a != &b);
  }
  expect("sizeof(expr)", 4, sizeof(1 + 2 * 3));
  expect("sizeof(str) include nul", 12, sizeof("hello\0world"));
  expect("sizeof(struct ptr)", sizeof(void*), sizeof(struct Undefined*));
  expect("sizeof(func ptr)", 8, sizeof(int (*)()));
  {
    int a[3] = {1, 2, 3};
    expect("array initializer", 1, a[0] == 1 && a[1] == 2 && a[2] == 3);
  }
  {
    int a[] = {1, 2};
    expect("array without size", 1, sizeof(a) == 2 * sizeof(int) && a[0] == 1 && a[1] == 2);
  }
  {
    int a[] = {1, 2, 3,};
    expect("array with last comma", 3 * sizeof(int), sizeof(a));
  }
  {
    struct {int x; int y;} s = {3};
    expect("struct initializer", 3, s.x + s.y);
  }
  {
    struct {int x; int y;} s = {3, 4,};
    expect("struct initializer with last comma", 7, s.x + s.y);
  }
  {
    struct {int x; int y;} s = {.y = 9};
    expect("struct initializer with member", 9, s.x + s.y);
  }
  {
    union {char x; int y;} u = {0x1234};
    expect("union initializer", 0x34, u.x);
  }
  {
    union {int y; char x;} u = {0x5678};
    expect("union initializer2", 0x5678, u.y);
  }
  {
    union {char x; int y;} u = {.y=0xabcd};
    expect("union initializer with member", 0xabcd, u.y);
  }
  {
    const int x = 123;
    expect("const", 123, x);
  }
  expect("file static", 456, s_val);
  expect("extern", 789, e_val);
  expect("?:", 2, 1 ? 2 : 3);

  return 0;
}

int e_val = 789;

int protodecl(int x) {
  return x * x;
}
