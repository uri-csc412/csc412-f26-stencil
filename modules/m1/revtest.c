#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reverse.h"

static int tests_run = 0;
static int tests_correct = 0;
static int tests_incorrect = 0;

#define USAGE "We detected no arguments. Running test suite...\n"
#define ERRORMSG "ERROR: The 2nd argument must match the the number of elements\n Please try again.\n"
#define EXIT_FAILURE 1

/**
 * checks if the elements are properly reversed 
 * and returns an error message if not
 */
void assert_equal(char** expected, char** actual, const char* message, int len) {
  tests_run++;
  int element_correct = 0;
  int element_incorrect = 0;

  for (int i = 0; i < len; i++) {
    if (!strcmp(expected[i], actual[i])) {
      element_correct++;
      if (element_correct == len) tests_correct++;
    } else {
      element_incorrect++;
      if (element_correct == len) tests_correct++;
      printf("\033[0;31m"); // set color to red
      printf(
          "[FAILED Test %d: %s]: At index: %d, expected string: %s, actual "
          "string: %s\n",
          tests_run, message, i, expected[i], actual[i]);
      printf("\033[0m"); // reset text to default color
    }
  }

  // if no elements are wrong, the test passed! Print a message that says this :)
  if (element_incorrect == 0){
    printf("\033[0;32m"); //set color to green
    printf("[PASSED Test %d: %s]\n", tests_run, message);
    printf("\033[0m"); //reset text to default color
  }

}

/**
 * checks if two strings are equal and returns an error message if not
 */
void assert_string_equal(char* expected, char* actual, char* message) {
    tests_run++;
    
    if (!strcmp(expected, actual)) {
        // strings are equal
        tests_correct++;
        printf("\033[0;32m"); // set color to green
        printf("[PASSED Test %d: %s]\n", tests_run, message);
        printf("\033[0m"); // reset text to default color
    } else {
        // strings are not equal
        tests_incorrect++;
        printf("\033[0;31m"); // set color to red
        printf("[FAILED Test %d: %s]: Expected string: '%s', Actual string: '%s'\n",
               tests_run, message, expected, actual);
        printf("\033[0m"); // reset text to default color
    }
}

void safe_strncpy(char* dest, const char* src, size_t size) {
    if (size > 0) {
        snprintf(dest, size, "%s", src); // snprintf ensures null-termination
    }
}

/**
 * runs tests for student implementation of reverse
 */
void test_array_reverse(void (*func)(char **arr, int size), int testnum) {
  printf("\n   Starting tests for reverse_arr%d\n", testnum);

  // test 1: test single element
  char* str1[] = {"hello"};
  func(str1, 1); // use the function pointer
  assert_equal(str1, str1, "one element test", 1);

  // test 3: test four elements (even number of elements)
  char* str3[] = {"hello", "my", "name", "is"};
  char* reverse3[] = {"is", "name", "my", "hello"};
  func(str3, 4); // use the function pointer
  assert_equal(str3, reverse3, "four element test", 4);

  // test 4: test five elements
  char* str4[] = {"i", "love", "systems", "csc", "412"};
  char* reverse4[] = {"412", "csc", "systems", "love", "i"};
  func(str4, 5); // use the function pointer
  assert_equal(str4, reverse4, "five element test", 5);

  // test 5: test 2 elements
  char* str5[] = {"abc\\ndef\\tghi", " abc "};
  char* reverse5[] = {" abc ", "abc\\ndef\\tghi"};
  func(str5, 2); // use the function pointer
  assert_equal(str5, reverse5, "test two elements with escaped chars and with leading/trailing whitespace", 2);

  // test 6: test empty element
  char* str6[] = {"hello NULL", ""};
  char* reverse6[] = {"", "hello NULL"};
  func(str6, 2); // use the function pointer
  assert_equal(str6, reverse6, "test empty input", 2);
}

/**
 * runs tests for student implementation of reverse
 */
void test_char_reverse(void (*func)(char*, int), int testnum) {
  printf("\n   Starting tests for test_char_reverse%d\n", testnum);

  // test 1: palindrome
  char str1[] = "madam"; // array of char (i.e, a string we can modify), which is modifiable
  char* rev1 = "madam";
  func(str1, 5); // should not change the string
  assert_string_equal(str1, rev1, "palindrome test");
    
  // test 2: test single string
  char str2[] = "hello"; // array of char (i.e, a string we can modify), which is modifiable
  char* rev2 = "olleh";
  func(str2, 5);
  assert_string_equal(str2, rev2, "test simple string");

  // test 3: simple special characters
  char str3[] = "hello, world!";
  char* rev3 = "!dlrow ,olleh";
  func(str3, 13);
  assert_string_equal(str3, rev3, "simple special characters test");

  // test 4: leading space characters
  char str4[] = "  leading space";
  char* rev4 = "ecaps gnidael  ";
  func(str4, 15);
  assert_string_equal(str4, rev4, "leading space characters test");

  // test 5: trailing space characters
  char str5[] = "trailing space  ";
  char* rev5 = "  ecapS gniliart";
  func(str5, 16);
  assert_string_equal(str5, rev5, "trailing space characters test");

  // test 6: trailing space characters
  char str11[] = " trailing space   ";
  char* rev11 = "   ecapS gniliart ";
  func(str11, 17);
  assert_string_equal(str11, rev11, "leading + trailing space characters test! (hard)");

  // test special: Unicode Characters
  char str6[] = "こんにちは";
  char* rev6 = "はちにんこ";
  func(str6, 6);
  assert_string_equal(str6, rev6, "Unicode Characters special test! (extra credit)");

  // test: string with Non-ASCII characters (UTF-8)
  char str8[] = "héllo";
  char* rev8 = "olléh";
  func(str8, 6);
  assert_string_equal(str8, rev8, "string with Non-ASCII characters test! (extra credit)");
}

/**
 * Print the number of tests passed
 */
void print_summary_stats() {
  printf("\n Your code ran %d tests. \n Your reverse functions passed %d of the 55 tests needed to get full credit.\n", tests_run, tests_correct);
}

/*
 * Runs tests in C for local development.
 *
 * Please complete the TODOs below. 
 */
int main(int argc, char** argv) {
  /*
    TODO Part 1:
      Uncomment each line below to test your 
      reverse array functions as you complete them.
  */ 
  test_array_reverse(&reverse_arr1,1);
  //test_array_reverse(&reverse_arr2,2);
  //test_array_reverse(&reverse_arr3,3);
  //test_array_reverse(&reverse_arr4,4);
  //test_array_reverse(&reverse_arr5,5);

  /*
    TODO Part 1: 
      Uncomment each line below to test your
      reverse char functions as you complete them.
  */ 
  //test_char_reverse(&reverse_str1,1);
  //test_char_reverse(&reverse_str2,2);
  //test_char_reverse(&reverse_str3,3);
  //test_char_reverse(&reverse_str4,4);
  //test_char_reverse(&reverse_str5,5);

  print_summary_stats();

  /*
  
  TODO Part 2: 

    if the number of args == 1, 
    then print the USAGE message to stdout,
    then run all of the test_array_reverse
    and test_char_reverse functions 
    you have completed, and run print_summary_stats()

    CLARIFICATION: copy and paste all of those 11 functions
  

  TODO Part 2: 

    handle bad example: ./revtest 3 hello world

    If the num_elements does not match the number of elements 
    provided as command line arguments, then 
      print the ERRORMSG to stderr, and finally 
      exit EXIT_FAILURE;
    
    handle good example: ./revtest 2 hello world
    
    If num_elements matches the number of elements
    provided as command line arguments, then 
    run your reverse_arr1 with the arguments provided
    and print the reversed array to stdout.
  */

  /*
  // Uncomment the if statements for Part 2
  // check for the number of arguments
  if ("checkArgsHere_del_me") {
  
  } else {
    // HINT: lookup the atoi function in C to get the num_elements

    // HINT: handle bad example - check if the num_elements matches the 2nd arg

    // char* arr[num_elements]; // initializes an array of character pointers

    // HINT: loop through the remaining arguments and add them to the array

    // reverse the array using reverse_arr1
    // finally, print the reversed array to stdout
  }
  */

  return 0;
}