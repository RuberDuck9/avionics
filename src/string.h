#include <math.h>

void string_reverse(char* string, int length);
int int_to_string(int x, char* string, int digits);
void float_to_string(float input, char* string, int decimals);


void string_reverse(char* string, int length){

	int i = 0, j = length - 1, temporary;
	while (i < j){
		temporary = string[i];
		string[i] = string[j];
		string[j] = temporary;
		i++;
		j--;
	}
}

int int_to_string(int x, char string[], int digits){
	
	int i = 0;
	while (x){
		string[i++] = (x % 10) + '0';
		x = x / 10;
	}

	while (i < digits){
		string[i++] = '0';
	}

	string_reverse(string, i);
	string[i] = '\0';
	return i;
}

void float_to_string(float input, char* string, int decimals){

	int integer_part = (int)input;

	float floating_part = input - (float)(integer_part);

	int i = int_to_string(integer_part, string, 0);

	if (decimals != 0){
		string[i] = '.';

		floating_part = floating_part * pow(10, decimals);

		int_to_string((int)floating_part, string + i + 1, decimals);
	}
}
