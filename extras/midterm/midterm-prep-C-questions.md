% Title: Midterm Practice Questions (C) | IN3230/IN4230 Høst 2026

% Author: Kristjon Ciko <kristjoc@uio.no>

% These are sample questions from the C quiz in Mentimeter during the 3rd
% plenary, to give students an idea of what the midterm could look like.

% ------


## 1. What does this function return?

    1  int f1(int a, int b) {
    2      return (a>b?a:b);
    3  }

A. compiler error

B. the smaller value of the two passed parameters

C. the greater value of the two passed parameters

D. runtime error

Answer: C

Explanation: The ternary operator inside the return statement evaluates if
(a>b) and returns 'a' if the condition is True. Otherwise, it returns 'b'.


## 2. Which is not a correct way to declare a string variable?

A. `char str = "Hei IN3230";`

B. `char *str = "Hei IN3230";`

C. `char str[20] = {'H', 'e', 'i', ' ', 'I', 'N', '3', '2', '3', '0', '\0'};`

D. `char str[] = "Hei IN3230";`

Answer: A

Explanation: `char str` declares only a 1-byte character and "Hei IN3230" cannot
be stored there.


## 3. Which function can be used to deallocate memory?

A. `dalloc()`

B. `release()`

C. `free()`

D. `dealloc()`

Answer: C

Explanation: From `man free()`: The `free()` function frees the memory
space pointed to by ptr, which must have been returned by a previous call to
`malloc()`, `calloc()` or `realloc()`. Otherwise, or if `free(ptr)` has already been
called before, undefined behavior occurs. If ptr is `NULL`, no operation is
performed.


## 4. In which segment does dynamic memory allocation take place?

A. BSS segment

B. stack

C. data segment

D. heap

Answer: D

Explanation: Both stack and heap are stored in a computer's RAM. While
a stack is used mainly for static memory allocation, a heap is used for dynamic
memory allocation.


## 5. What is not a valid command with this declaration?

    1 char *string[20] = {"one", "two", "three"};

A. `printf("%c", string[1][2]);`

B. `printf("%s", string[1][2]);`

C. `printf(string[1]);`

D. `printf("%s", string[1]);`

Answer: B

Explanation: Using the wrong format specifier invokes undefined behavior,
Segmentation fault in this case. `%s` tells `printf` that the argument is a
pointer to string and will try to print until the `\0` null-terminated
character, but `string[1][2]` is a char.


## 6. Void pointer `vptr` is assigned the address of float variable `g`. What is a valid way to dereference `vptr` to assign its pointed value to a float variable named `f` later in the program?

    1  float g;
    2  void *vptr = &g;

A. `f = *(float)vptr;`

B. `f = (float)*vptr;`

C. `f = *(float *)vptr;`

D. `f = (float *)vptr;`

Answer: C

Explanation: Since `vptr` is a void pointer, first we cast it to the right
pointer type, `(float *)`, and then get the pointed value using the dereference
operator (`*`).


## 7. What does the `strcmp(str1, str2);` function return?

A.
• true (1) if str1 and str2 are the same
• NULL if str1 and str2 are not the same

B.
• 0 if str1 and str2 are the same
• a negative number if str1 is less than str2
• a positive number if str1 is greater than str2

C.
• true (1) if str1 and str2 are the same
• false (0) if str1 and str2 are not the same

D.
• 0 if str1 and str2 are the same
• a negative number if str2 is less than str1
• a positive number if str2 is greater than str1

Answer: B

Explanation: From `man strcmp()`: The `strcmp()` function compares the two strings
s1 and s2. It returns an integer less than, equal to, or greater than zero if s1
is found, respectively, to be less than, to match, or be greater than s2.


## 8. How many times does the code inside the while loop get executed in this program?

    1  void main() {
    2  int x=1;
    3      while (x++<100) {
    4          x*=x;
    5          if (×<10) continue;
    6          if (x>50) break;
    7      }
    8  }

A. 5

B. 50

C. 100

D. 3

Answer: D

Explanation:
First iteration: Initial value x=1; x++ inside while() increases the value to 2;
Line 4 changes x to 4;
Second iteration: x++ inside while() increases the value to 5; Line 4 changes
the value to 25;
Third iteration: x++ inside while() increases the value to 26; Line 4 changes
the value of x to 676, which is bigger than 50;
while() breaks after 3 iterations.


## 9. Which choice is an include guard for the header file mylib.h?

A.
```
#ifndef MYLIB_H
#define MYLIB_H
//mylib.h content
#endif /* MYLIB_H */
```

B.
```
#define MYLIB_H
#include "mylib.h"
#undef MYLIB_H
```

C.
```
#ifdef MYLIB_H
#define MYLIB_H
//mylib.h content
#endif /* MYLIB_H */
```

D.
```
#ifdef MYLIB_H
#undef MYLIB_H
//mylib.h content
#endif /* MYLIB_H */
```

Answer: A

Explanation: If MYLIB_H macro is not defined before, define the macro and add
the content of the header.


## 10. What is the sequence printed by the following program?

    1  #include <stdio.h>
    2
    3  void print_3_ints(int *ip)
    4  {
    5      printf("%d %d %d ", ip[0], ip[1], ip[2]);
    6  }
    7
    8
    9  int main(void)
    10 {
    11     int array[] = { 5, 23, 119, 17 };
    12     int *p, *q, d;
    13
    14     p = array; q = p + 1;
    15
    16     print_3_ints(q);
    17
    18     d = *(p++);
    19     printf("%d ", d);
    20
    21     d = ++(*p);
    22     printf("%d ", d);
    23
    24     printf("\n");
    25     return 0;
    26 }

A. 23 119 17 5 24

B. 23 119 17 23 24

C. 24 120 18 6 25

D. 25 120 18 25 26

Answer: A

Explanation: Line 14: `p` points to array[0] = 5, and `q` points to the entry at
the next address, 23. Line 16 prints 23 119 17. Line 18: `d` is assigned with the value
that `p` points to, 5, and then `p` points to the next address in the array. So, Line
19 prints 5. Now `p` points to the entry 23 and Line 21 increments the dereferenced value
to 24 and then assigns it to `d`. Finally, Line 22 prints 24.


## 11. What is the output of the following program?

    1  #include <stdio.h>
    2
    3  void func(int *ptr)
    4  {
    5      *ptr = 30;
    6  }
    7
    8  int main()
    9  {
    10     int y = 20;
    11     func(&y);
    12     printf("%d\n", y);
    13
    14     return 0;
    15 }

A. 20

B. 30

C. Compiler error

D. Runtime error

Answer: B

Explanation: The address of `y` is passed to `func()` as a pointer and the value
pointed by the pointer is changed to 30.


## 12. What is the output of the following program?

    1  #include <stdio.h>
    2  #include <stdlib.h>
    3
    4  int main()
    5  {
    6      int i, numbers[1];
    7      numbers[0] = 15;
    8      free(numbers);
    9      printf("Stored integers are ");
    10     printf("numbers[%d] = %d ", 0, numbers[0]);
    11
    12     return 0;
    13 }

A. Runtime error

B. Compilation error

C. 0

D. Garbage value

Answer: A

Explanation: The program can be compiled with a warning that a non-heap memory
is trying to be freed; `numbers[1]` array is statically allocated in stack and
cannot be freed. However, when the program is executed there is a runtime error.


## 13. What is the error of this program?

    1  #include <stdio.h>
    2  #include <stdlib.h>
	3  #include <string.h>
    4
    5  int main()
    6  {
    7      char *ptr;
    8      *ptr = (char)malloc(8);
    9      strcpy(ptr, "RAM");
    10     printf("%s", ptr);
    11     free(ptr);
    12     return 0;
    13 }

A. Error in `strcpy()` statement

B. Error in `*ptr = (char)malloc(8);`

C. Error in `free(ptr);`

D. No error

Answer: B

Explanation: `malloc(8)` needs to be casted to a pointer to char type,
`(char*)malloc(8)`.


## 14. What is the output of the following program?

    1  #include <stdio.h>
    2
    3  struct result{
    4      char sub[20];
    5      int marks;
    6  };
    7
    8  int main()
    9  {
    10     struct result res[] = {
    11          {"IN3230",100},
    12    		{"IN4230",90},
    13    		{"Norsk",85}};
    14
    15     printf("%s ", res[1].sub);
    16
    17     printf("%d\n", (*(res+2)).marks);
    18
    19     return 0;
    20 }

A. IN3230 100

B. IN4230 85

C. IN4230 90

D. Norsk 100

Answer: B

Explanation: Line 15 prints the second subject with index [1], IN4230, and
Line 17 prints the third marks with index [2], 85.


## 15. What is the output of this code snippet?

    1  void main()
    2  {
    3      struct bitfields {
    4          int bits_1: 2;
    5          int bits_2: 9;
    6          int bits_3: 6;
    7          int bits_4: 1;
    8      }bit;
    9
    10     printf("%d\n", sizeof(bit));
    11 }

A. 2

B. 3

C. 4

D. 0

Answer: C

Explanation:  2 + 9 + 6 + 1 = 18 bit, so 3 bytes x 8 bit = 24 bits need to be
used to store the `struct bitfields` type in the memory. Because
`__attribute__((packed))` is not used here, 8 bits of padding occurs and
everything can be stored in 32 bits = 4 bytes


## 16. In which line is the BUG in the following program?

    1  #include <stdio.h>
    2
    3  int main()
    4  {
    5      int a, *ptr;
    6      a = 25;
    7      *ptr = a + 5;
    8
    9      return 0;
    10 }

Answer: Line 7

Explanation: `ptr` pointer is not initialized and cannot be dereferenced.


## 17. In which line is the BUG in the following program?

    1  #include <stdio.h>
    2  struct var {
    3      int value;
    4      int *address;
    5  };
    6
    7  int main()
    8  {
    9      struct var y;
    10     int a = 10;
    11     int *ptr = &a;
    12
    13     y.value = *ptr;
    14     (&y)->address = ptr;
    15
    16     printf("value %d\n", y.value);
    17     printf("address %p\n", y->address);
    18     return 0;
    19  }

Answer: Line 17

Explanation: `y` is not a pointer hence the address value of `y` should be accessed
with a dot (.) operator, not an arrow (->).


## 18. What are the two mistakes in the following program?

    1   #include <stdio.h>
    2
    3   int main () {
    4       const char src[8] = "IN3230";
    5       char dest[8];
    6
    7       strcpy(dest, "Hellllo");
    8
    9       memcpy(dest, src, strlen(src));
    10
    11      printf("%s\n", dest);
    12      return(0);
    13  }

Answer:
1. `#include <string.h>`
2. `memcpy(dest, src, strlen(src) + 1);`

Explanation: `memcpy()` is defined in the "string.h" header; `strlen()` does not count
the null terminator at the end of the string "IN3230".
