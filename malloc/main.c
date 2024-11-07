#include <stdio.h>
#include <stdlib.h>

void* my_malloc(size_t size);
void my_free(void *ptr);
void print(void);
int main(void)
{
    int *str1 = my_malloc(0);
    printf("%p\n", str1);
    int *i = my_malloc(sizeof(int));
    printf("%p\n", i);
    int *j = my_malloc(sizeof(int));
    printf("%p\n", j);
    char *str = my_malloc(6*sizeof(char));
    if (!str || !i || !j)
    {
        return 1;
    }
//    print();
    my_free(i);
    my_free(j);
    //print();
    
    void *p = my_malloc(4096);
    printf("%p\n", p);
    my_free(p);
  
   // print();
    my_free(str1);
    my_free(str);
    //print();
    return 0;
}
