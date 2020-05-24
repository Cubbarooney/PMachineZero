#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_VAR_LEN 12
#define MAX_NUM_LEN 6//5+1(for terminating)


typedef enum
{
	nulsym = 1, identsym, numbersym, plussym, minussym, multsym, slashsym, oddsym,
	eqsym, neqsym, lessym, leqsym, gtrsym, geqsym, lparentsym, rparentsym, commasym,
	semicolonsym, periodsym, becomessym, beginsym, endsym, ifsym, thensym, whilesym,
	dosym, callsym, constsym, varsym, procsym, writesym, readsym, elsesym
} token_type;

typedef enum{comment = 0, longword = -1, wrongstart = -2, invaldsym = -3, endOfFile = -4 } errors;

typedef enum{false, true} bool;

char whitespace[] = {' ', '\t', '\v'};
char newlines[] = {'\n', '\r'};

int DFA(char, FILE*, FILE*);
void printLexeme(FILE*, FILE*, char*, int);
void setWord(char*);
int checkWord(char*);
bool isWhitespace(char);
bool isNewline(char);

char word[MAX_VAR_LEN];
char numb[MAX_NUM_LEN];
int lineNumber = 0;

int main()
{
	int tokenValue;
	bool running = true;
	int result = 0;
	char current;

	FILE *input = fopen("input.txt", "r");
	FILE *outputCleanSource = fopen("cleaninput.txt", "w");
	FILE *outputLexeme = fopen("lexemetable.txt", "w");
	FILE *outputTokenList = fopen("tokenlist.txt", "w");

    fprintf(outputLexeme, "Lexeme      Token Type\n");

	while(running)
	{
		current = fgetc(input);
		if(current == EOF) break;
		//printf("%c\n",current);
		tokenValue = DFA(current, input, outputCleanSource);
		//printf("==%d==\n", tokenValue);

        switch(tokenValue)
        {
            case(endOfFile):
                result = 0;
                running = false;
                break;
            case(invaldsym):
                result = tokenValue;
                printf("ERROR: invalid symbol\nLINE: %d\n", lineNumber);
                running = false;
                break;
            case(wrongstart):
                result = tokenValue;
                printf("ERROR: invalid variable name\nLINE: %d\n", lineNumber);
                running = false;
                break;
            case(longword):
                result = tokenValue;
                printf("ERROR: value too long\nLINE: %d\n", lineNumber);
                running = false;


            case(nulsym):
                setWord("NULL");
                break;
            case(numbersym):
                setWord(numb);
                break;
            case(plussym):
                setWord("+");
                break;
            case(minussym):
                setWord("-");
                break;
            case(multsym):
                setWord("*");
                break;
            case(slashsym):
                setWord("/");
                break;
            case(eqsym):
                setWord("=");
                break;
            case(neqsym):
                setWord("<>");
                break;
            case(lessym):
                setWord("<");
                break;
            case(leqsym):
                setWord("<=");
                break;
            case(gtrsym):
                setWord(">");
                break;
            case(geqsym):
                setWord(">=");
                break;
            case(lparentsym):
                setWord("(");
                break;
            case(rparentsym):
                setWord(")");
                break;
            case(semicolonsym):
                setWord(";");
                break;
            case(periodsym):
                setWord(".");
                running = false;
                result = 0;
                break;
            case(commasym):
                setWord(",");
                break;
            case(becomessym):
                setWord(":=");
                break;
        }

        if(tokenValue > 0){
            //printf("=%s\n", word);
            printLexeme(outputLexeme, outputTokenList, word, tokenValue);}
        if(tokenValue == 2 || tokenValue == 3)
            fprintf(outputTokenList, "%s ", word);


	}

    fprintf(outputCleanSource, "\n");
    fprintf(outputTokenList, "\n");

	fclose(input);
	fclose(outputCleanSource);
	fclose(outputLexeme);
	fclose(outputTokenList);

	return result;
}

int DFA(char cin, FILE *in, FILE *out)
{
    char c = cin;

	int i = 1;
	//char word[MAX_VAR_LEN];
	//char numb[MAX_NUM_LEN];

    while(isWhitespace(c)){
        fprintf(out, "%c", c);
        //printf("yo\n",c);
        c = getc(in);
    }
	//advance through blankspace.

	if(c == EOF)
        return -4;

	if(isalpha(c))
	{//we got us a variable or a reserved word!
		word[0] = c;
		while(isalpha(c = getc(in)))//or digit
		{

			word[i] = c;
			i++;
			if(i == MAX_VAR_LEN)
				return longword;//too long of a word.
		}
		word[i] = '\0';//double check the value of EOS
		ungetc(c, in);

		//printf("\n%c",c);

		fprintf(out, "%s", word);

		return checkWord(word);
	}
	else
	{
		if(isdigit(c))
		{//we got us a number!
		    numb[0] = c;
		    while(isdigit(c = getc(in)))
            {
                numb[i] = c;
                i++;
                if(i == MAX_NUM_LEN)
                    return longword;
            }
            if(isalpha(c))
                return wrongstart;
            numb[i] = '\0';
            ungetc(c, in);

            fprintf(out, "%s", numb);

            return numbersym;
		}
		else
		{
		    if(c != '/')
                fprintf(out, "%c", c);
            //printf("==%c==\n", c);
            //printf("s%cs",c);
			switch(c)
			{
				case '+':
				    return plussym;
				case '-':
				    return minussym;
				case '*':
				    return multsym;
				case '/'://NB: got to check if this is a comment or division
				    c = fgetc(in);

				    if(c == '*')
                    {
                        while(true)
                        {
                            c = fgetc(in);
                            isNewline(c);
                            if(c == '*')
                            {
                                c = fgetc(in);
                                if(c == '/')
                                {
                                    fprintf(out, " ");//this is for comments and guarentee that there is a space between the comment and the next item.
                                    return 0;
                                }
                                else
                                {
                                    ungetc(c, in);
                                }
                            }
                        }
                    }
                    else
                    {
                        fprintf(out, "%c", '/');
                        ungetc(c, in);
                        return slashsym;
                    }
				case '=':
				    return eqsym;
				case '<'://got to check if it is <, <=, or <>
				    c = fgetc(in);
				    if(c == '>')
				    {
				        fprintf(out, "%c", c);
				        return neqsym;
				    }
				    else if(c == '=')
                    {
                        fprintf(out, "%c", c);
                        return leqsym;
                    }
                    else
                    {
                        ungetc(c, in);
                        return lessym;
                    }
				case '>'://got to check if it is > or >=
				    c = fgetc(in);
				    if(c == '=')
                    {
                        fprintf(out, "%c", c);
                        return geqsym;
                    }
                    else
                    {
                        ungetc(c, in);
                        return gtrsym;
                    }
				case '(':
                    return lparentsym;
				case ')':
				    return rparentsym;
				case ',':
				    return commasym;
				case ';':
				    //printf("!\n");
				    return semicolonsym;
				case '.':
				    return periodsym;
				case ':'://must be :=
				    c = fgetc(in);
				    if(c != '=')
                        return invaldsym;
                    fprintf(out, "%c", c);
                    return becomessym;
                case '\a'://Due to how get char works, this won't actually happen :(
                    printf("\a\nSorry! But '\\a' is not allowed in this code!\nYou rock though!\n");
                    return invaldsym;//:D
            }
		}
	}

	return invaldsym;//If we failed for some reason to identify what this thing is, we are in error.
}

void printLexeme(FILE* lexFile, FILE* tokFile, char* name, int value)
{
    //printf("-%d\n", value);
    fprintf(lexFile, "%-11s %d\n", name, value);
    fprintf(tokFile, "%d ", value);
}

void setWord(char *in)
{
    strcpy(word, in);
}

int checkWord(char *in)
{
    switch(in[0])
	{
		case 'b':
			if(!strcmp("begin", in)) return beginsym;
			break;
		case 'c':
			if(!strcmp("call", in)) return callsym;
			else if(!strcmp("const", in)) return constsym;
			break;
		case 'd':
			if(!strcmp("do", in)) return dosym;
			break;
		case 'e':

			if(!strcmp("else", in)) return elsesym;
			else if(!strcmp("end", in)) return endsym;
			break;
		case 'i':
		    if(!strcmp("if", in)) return ifsym;
			break;
		case 'o':
			if(!strcmp("odd", in)) return oddsym;
			break;
		case 'p':
			if(!strcmp("procedure", in)) return procsym;
			break;
		case 'r':
			if(!strcmp("read", in)) return readsym;
			break;
        case 't':
            if(!strcmp("then", in)) return thensym;
            break;
        case 'v':
            if(!strcmp("var", in))return varsym;
            break;
		case 'w':
			if(!strcmp("while", in)) return whilesym;
			else if(!strcmp("write", in)) return writesym;
			break;
	}

	return identsym;
}

bool isWhitespace(char c)
{
    int i;
    int len = sizeof(whitespace)/sizeof(char);
    for(i = 0; i < len; i++)
        if(whitespace[i] == c)
            return true;
    return isNewline(c);
}

bool isNewline(char c)
{
    int i;
    int len = sizeof(newlines)/sizeof(char);
    for(i = 0; i < len; i++)
        if(newlines[i] == c)
        {
            lineNumber++;
            return true;
        }
    return false;
}
