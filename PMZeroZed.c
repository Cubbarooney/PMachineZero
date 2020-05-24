/*PM/0 Build Alpha
* Jacob "Cubby" Rubenstein
* 09/12/15
* For System Software with Matthew Gerber
* Assignment 1
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef struct{
    int op;
    int l;
    int m;
} instruction;

typedef enum
{
	nulsym = 1, identsym, numbersym, plussym, minussym, multsym, slashsym, oddsym,
	eqsym, neqsym, lessym, leqsym, gtrsym, geqsym, lparentsym, rparentsym, commasym,
	semicolonsym, periodsym, becomessym, beginsym, endsym, ifsym, thensym, whilesym,
	dosym, callsym, constsym, varsym, procsym, writesym, readsym, elsesym
} token_type;

typedef enum{comment = 0, longword = -1, wrongstart = -2, invaldsym = -3, endOfFile = -4 } errors;

typedef enum{false, true} bool;

//PMZeroAlpha
int loadIR(FILE*);
int fetch(FILE*);
int execute(FILE*);
int pop();
void push(int);
int base(int);
const char* getOP(int);
bool checkIsValid(FILE*, instruction*);
void lexiError(FILE*, instruction*, char*);
void sioError(FILE*, instruction*, int);
void readInput();
void nullArray(int, int*, int);
void printInstructions();//DEBUGGING
void printStack();//DEBUGGING
void outputStack(FILE*);
void printReg(FILE*);
void printLine(FILE*);
//PMSZeroBeta
int scanner();
int DFA(char, FILE*, FILE*);
void printLexeme(FILE*, FILE*, char*, int);
void setWord(char*);
int checkWord(char*);
bool isWhitespace(char);
bool isNewline(char);
//PMPZeroGamma

//PMZeroZed
void printFile(char*);

#define MAX_STACK_HEIGHT 2000
#define MAX_CODE_LENGTH 500
#define MAX_LEXI_LEVELS 3
#define MAX_VAR_LEN 12
#define MAX_NUM_LEN 6


//PMZeroAlpha
int stack[MAX_STACK_HEIGHT];
instruction *code[MAX_CODE_LENGTH];
//TODO Change the way this is implemented. The bps are all stored in the stack!
int startOfStacks[MAX_STACK_HEIGHT / 4 + 1];//MAX_LEXI_LEVELS];
//Stack Pointer
int sp = 0;
//Program Counter
int pc = 0;
//Base Pointer - Base of the
int bp = 1;
instruction *ir;
//Top Lexi Level (1 is the main lexi)
int numOfLexis = 1;
int CODE_LENGTH = 0;
bool running = true;
//Print to Console Toggles
bool printT = true;//false;//Token
bool printS = true;//false;//SymbolTable
bool printM = true;//false;//Machine Code
bool printA = true;//false;//disassembled code
bool printV = true;//false;//stack trace

//PMSZeroBeta
char whitespace[] = {' ', '\t', '\v'};
char newlines[] = {'\n', '\r'};
char word[MAX_VAR_LEN];
char numb[MAX_NUM_LEN];
int lineNumber = 0;

//Files for the parser
FILE outputCodeFile;
FILE outputTableFile;
FILE outputTableFilePretty;
FILE input;

int main(int argc, char *argv[]){
    int i;
    int exit_status = 0;
    int result = 0;
    FILE *outputStackTrace;
    FILE *outputACode;

    //PMZeroSetup
    for(i = 1; i < argc; i++)
    {
        if(strcmp("-t", argv[i]) == 0)
            printT = true;
        else if(strcmp("-s", argv[i]) == 0)
            printS = true;
        else if(strcmp("-m", argv[i]) == 0)
            printM = true;
        else if(strcmp("-a", argv[i]) == 0)
            printA = true;
        else if(strcmp("-v", argv[i]) == 0)
            printV = true;
    }

    //PMSZeroBeta
    result = scanner();
    if(printT) printFile("tokenlist.txt");


    //Initialize all cells of the stack store to 0
    nullArray(MAX_STACK_HEIGHT, stack, 0);
    //set all of the instructions to null (explicit setting)
    for(i = 0; i < MAX_CODE_LENGTH; i++)
        code[i] = NULL;
    //currently, all of the possible stacks are nonexistant
    //As such, they don't start yet!
    nullArray(MAX_LEXI_LEVELS, startOfStacks, 0);
    startOfStacks[0] = bp;

    //Read the input file "mcode.txt"
    readInput();

    //Open an output file and put the "title" into the file!
    outputStackTrace = fopen("stacktrace.txt", "w");
    outputACode = fopen("acode.txt", "w");

    fprintf(outputACode, "Line   OP      L M\n");

    //create the IR and print each instruction to the output file
    exit_status = loadIR(outputACode);

    //place the titles for the stacktrace now that we have loaded the program.
    if(exit_status == 0){
        fprintf(outputStackTrace, "Line   OP      L M     PC  BP  SP   Stack\n");
        fprintf(outputStackTrace,   "Initial Values   -->   %-3d %d   %-3d   ", pc, bp, sp);
        outputStack(outputStackTrace);
        printLine(outputStackTrace);
    }

    //CORE
    while(exit_status == 0){
        exit_status = fetch(outputStackTrace);
        //SEVERE ERROR, END IMEDIATELY!
        if(exit_status == 2) break;
        exit_status = execute(outputStackTrace);

    }
    exitCheck(outputStackTrace, exit_status);
    fclose(outputStackTrace);
    fclose(outputACode);

    if(printA) printFile("acode.txt");
    if(printV) printFile("stacktrace.txt");

    //todo consolodate error output
}

int loadIR(FILE* output)
{
    int i, exit_status = 0;
    bool error = false;
    for(i = 0; i < CODE_LENGTH; i++){
        ir = code[i];

        fprintf(output, "%-3d    %s(%02d) %d %d\n", i, getOP(ir->op), ir->op, ir->l, ir->m);


        error = !checkIsValid(output, ir);

        if(error) exit_status = 4;
    }
    return exit_status;
}

int fetch(FILE* output)
{
    //FETCH
    ir = code[pc];
    //MAKE SURE THERE IS CODE AT THIS SPOT
    if(ir == NULL) return 2;
    //CHECK FOR PC ERROR
    if(pc >= CODE_LENGTH) return 1;

    //output to file the current instruction
                 //"Line   OP      L  M    PC  BP  SP    Stack
    fprintf(output,"%-3d    %s(%02d) %d %-3d   ", pc, getOP(ir->op), ir->op, ir->l, ir->m);

    //increment the pc by one!
    pc++;

    return 0;
}

int execute(FILE* output){
    int a;
    switch(ir->op){
        //PUSH LITERAL 'M'
        case(1):
            push(ir->m);
            break;
        //OPR
        case(2):
            switch(ir->m){
                //RETURN
                case(0):
                    sp = bp - 1;
                    pc = stack[sp + 4];
                    bp = stack[sp + 3];
                    startOfStacks[numOfLexis - 1] = 0;
                    numOfLexis--;
                    break;
                //NEGATE TOP
                case(1):
                    stack[sp] *= -1;
                    break;
                //ADD TOP TWO
                case(2):
                    a = pop();
                    stack[sp] += a;
                    break;
                //SUBTRACT TOP FROM TOP MINUS ONE
                case(3):
                    a = pop();
                    stack[sp] -= a;
                    break;
                //MULTIPLY TOP TWO
                case(4):
                    a = pop();
                    stack[sp] *= a;
                    break;
                //INTEGER DIVIDE SECOND BY TOP
                case(5):
                    a = pop();
                    stack[sp] /= a;
                    break;
                //ONE IF TOP IS ODD, ELSE 0
                case(6):
                    push(pop() % 2);
                    break;
                //MOD SECOND BY TOP
                case(7):
                    a = pop();
                    stack[sp] %= a;
                    break;
                //TOP TWO EQUAL
                case(8):
                    a = pop();
                    if(a == pop()) push(1);
                    else push(0);
                    break;
                //TOP TWO NOT EQUAL
                case(9):
                    a = pop();
                    if(a != pop()) push(1);
                    else push(0);
                    break;
                //TOP VALUE >  SECOND VALUE
                case(10):
                    a = pop();
                    if(a > pop()) push(1);
                    else push(0);
                    break;
                //TOP VALUE >= SECOND VALUE
                case(11):
                    a = pop();
                    if(a >= pop()) push(1);
                    else push(0);
                    break;
                //TOP VALUE <  SECOND VALUE
                case(12):
                    a = pop();
                    if(a < pop()) push(1);
                    else push(0);
                    break;
                //TOP VALUE <= SECOND VALUE
                case(13):
                    a = pop();
                    if(a <= pop()) push(1);
                    else push(0);
                    break;
            }
            break;
        //LOAD
        case(3):
            push(stack[base(ir->l) + ir->m]);
            break;
        //STORE TOP OF THE STACK 'L' LEVES DOWN IN THE 'M' POSITION
        case(4):
            stack[base(ir->l) + ir->m] = pop();
            break;
        //CALL PROCEDURE AT 'M'
        case(5):
            stack[sp + 1] = 0;
            stack[sp + 2] = base(ir->l);
            stack[sp + 3] = bp;
            stack[sp + 4] = pc;
            bp = sp + 1;
            pc = ir->m;
            numOfLexis++;
            startOfStacks[numOfLexis - 1] = bp;
            //check if a stack was added illegally
            //if(numOfLexis > MAX_LEXI_LEVELS){
            //   return 3;
            //}
            break;
        //ALLOCATE 'M' VARIABLES
        case(6):
            sp += ir->m;
            break;
        //UNCONDITIONALLY JUMP
        case(7):
            pc = ir->m;
            break;
        //JUMP IF TOP OF THE STACK IS ZERO
        case(8):
            if(pop() == 0){
                pc = ir->m;
            }
            break;
        //OUTPUT THE TOP OF THE STACK
        case(9):
            printf("\n%d\n", pop());
            break;
        //GET AN INPUT FROM USER
        case(10):
            scanf("%d", &a);
            push(a);
            break;
        //HALT
        case(11):
            pc = 0;
            bp = 0;
            sp = 0;
            printReg(output);
            fprintf(output, "\n");
            return -1;
    }
    //output the updated pointers
    printReg(output);

    //output the stack to the file
    outputStack(output);

    return 0;
}

int exitCheck(FILE* output, int exit_status){
    printLine(output);
    if(exit_status == -1) exit_status = 0;
    fprintf(output, "\nExit Status: %d\n", exit_status);
    //Any comments, based on the exit_status
    switch(exit_status){
        case(1):
            fprintf(output, "\nERROR: PC EXCEEDED CODE LENGTH\n");
            fprintf(output, "    PC = %d\n", pc);
            fprintf(output, "    CODE LENGTH = %d\n", CODE_LENGTH);
            fprintf(output, " NB: PC IS INDEXED AT ZERO, CODE LENGTH IS INDEXED AT ONE\n");
            break;
        case(2):
            fprintf(output, "ERROR: NO CODE AT PC\n");
            fprintf(output, "    PC = '%d'\n", pc);
            break;
        case(3):
            fprintf(output, "\nERROR: A LEXI WAS ATTEMPTED TO BE ADDED BEYOND THE LIMIT\n");
            fprintf(output, "    TOTAL AMOUNT OF LEXIS = %d\n", numOfLexis);
            fprintf(output, "    MAXIMUM AMOUNT OF LEXIS = %d\n", MAX_LEXI_LEVELS);
            break;
        case(4):
            fprintf(output, "\nERROR: PLEASE SEE ABOVE FOR CODE ERRORS\n");
            break;
    }
    //fclose(output);
}

int pop(){
    //return the top item of the stack
    int spam = stack[sp];
    sp--;
    return spam;
}

void push(int eggs){
    //add the input to the stack
    sp++;
    stack[sp] = eggs;
}

int base(int l){
    int b = bp;
    while(l > 0){
        b = stack[b + 1];
        l--;
    }
    return b;
}

void readInput(){
    FILE *file;
    int spam, eggs = 0;
    file = fopen("mcode.txt","r");
    fscanf(file, "%d", &spam);//first op code
    while(!feof(file)){
        instruction *temp = malloc(sizeof(instruction));
        temp->op = spam;
        fscanf(file, "%d", &spam);//l
        temp->l = spam;
        fscanf(file, "%d", &spam);//m
        temp->m = spam;
        //addInstruction(temp);
        code[eggs] = temp;
        eggs++;

        fscanf(file, "%d", &spam);//next op

    }
    CODE_LENGTH = eggs;
    fclose(file);

    //printInstructions();
}

void nullArray(int max, int* array, int defaultValue){
    int i;
    for(i = 0; i < max; i++)
        array[i] = defaultValue;
}

const char* getOP(int op){
    switch(op){
            case(1):
                return "LIT";
            case(2):
                return "OPR";
            case(3):
                return "LOD";
            case(4):
                return "STO";
            case(5):
                return "CAL";
            case(6):
                return "INC";
            case(7):
                return "JMP";
            case(8):
                return "JPC";
            case(9):
            case(10):
            case(11):
                return "SIO";
            default:
                return "NOP";
    }
    return NULL;
}

bool checkIsValid(FILE *output, instruction *in){
    bool flag = true;
    switch(in->op){
            case(1):
            case(2):
            case(6):
            case(7):
            case(8):
                if(in->l != 0){
                    lexiError(output, in, "Zero\n");
                    flag = false;
                }
                break;
            case(3):
            case(4):
            case(5):
                if(in->l > MAX_LEXI_LEVELS){

                    lexiError(output, in, "not greater than ");
                    fprintf(output, "%d\n", MAX_LEXI_LEVELS);
                    flag = false;
                }
                break;
            case(9):
                if(in->l != 0){
                    lexiError(output, in, "Zero\n");
                    flag = false;
                }
                if(in->m != 1){
                    sioError(output, in, 1);
                    flag = false;
                }
                break;
            case(10):
                if(in->l != 0){
                    lexiError(output, in, "Zero\n");
                    flag = false;
                }
                if(in->m != 2){
                    sioError(output, in, 2);
                    flag = false;
                }
                break;
            case(11):
                if(in->l != 0){
                    lexiError(output, in, "Zero\n");
                    flag = false;
                }
                if(in->m != 3){
                    sioError(output, in, 3);
                    flag = false;
                }
                break;
    }
    return flag;
}

void sioError(FILE *file, instruction *in, int i){
    fprintf(file, "ERROR: OP Code '%d' requires M to equal '%d'\n", in->op,i);
}

/*@params - FILE* file:
* handles the output when there is an error with the L value of the instruction*/
void lexiError(FILE *file, instruction *in, char* str){
    fprintf(file, "ERROR: Inputed lexi level '%d' is invalid. For OP code '%d', lexi level must be %s", in->l, in->op, str);
}

/*Prints the all instructions to the console
* For debugging*/
void printInstructions(){
    printf("\n=======================\nOUTPUTTING INSTRUCTIONS\n=======================\n");
    int i = 0;
    while(code[i] != NULL)
    {
        printf("%d %d %d\n", code[i]->op, code[i]->l, code[i]->m);
        i++;
    }
    printf("=======================\n");
}
/*See outputStack()
* Does the same thing, but prints to the console
* For debugging*/
void printStack(){
    printf("\n");//=======================\nOUTPUTTING STACK\n=======================\n");
    int i;
    for(i = 1; i <= sp; i++){
        if(i == bp){
            printf("| ");
        }
        printf("%02d ",stack[i]);
    }
    printf("\n");//=======================\n");
}

/*@params - FILE* output: file being written to
* This functions takes the current stack and outputs all the values it contains
* It also puts a vertical line BEFORE the base pointer's position.
* Note, stack[0] is not outputed!*/
void outputStack(FILE *output){
    int i;
    int j = 0;
    for(i = 1; i <= sp; i++){
        if(i == startOfStacks[j]){
            j++;
            fprintf(output, "| ");
        }
        //if(i == bp) fprintf(output, "| ");
        fprintf(output, "%d ", stack[i]);
    }
    fprintf(output, "\n");
}

/*@params - FILE* output: file being written to
* This functions takes the current registers and outputs each value
* correctly formatted.*/
void printReg(FILE* output){
    fprintf(output, "%-3d %d   %-3d  ", pc, bp, sp);
}

/*@params - FILE* output: The file being written to
* This functions just outputs a line of '=' symbols to the output file.
* For visual organization.*/
void printLine(FILE* output){
    fprintf(output,   "======================================\n");
}

//PMSZeroBeta
int scanner()
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

//PMZeroZed
void printFile(char* filename)
{
    FILE *file;
    char c;
    file = fopen(filename, "r");

    c = fgetc(file);
    while(c != EOF)
    {
        printf("%c", c);
        c = getc(file);
    }

    fclose(file);
}
