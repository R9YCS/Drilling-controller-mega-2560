#include <iostream>
int max = 0;
int c = 0;

int N,M;
int main() {


for (int i in range (N:M)) {
 if ((i%2!=i%6) and (i%9==0 or i%10==9 or i%11==0)) {
c = c + 1;
max = i;
 }
}
print(c,max);
}
