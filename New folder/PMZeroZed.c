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

#define MAX_TABLE_SIZE 100
#define MAX_STACK_HEIGHT 2000
#define MAX_CODE_LENGTH 500
#define MAX_LEXI_LEVEL 3
#define MAX_VAR_LEN 12
#define MAX_NUM_LEN 6

typedef struct{
    int op;
    int l;
    int m;
} instruction;

typedef struct symbol
{
  int kind;	//const = 1, var = 2, proc = 3
  char name[MAX_VAR_LEN];//name is at most 11 chars long
  int val;	//value
  int level;	//L
  int addr;	//M
} symbol;

typedef struct codeLine
{
  int op;
  int l;
  int m;
} codeLine;

typedef enum
{
	nulsym = 1, identsym, numbersym, plussym, minussym, multsym, slashsym, oddsym,
	eqsym, neqsym, lessym, leqsym, gtrsym, geqsym, lparentsym, rparentsym, commasym,
	semicolonsym, periodsym, becomessym, beginsym, endsym, ifsym, thensym, whilesym,
	dosym, callsym, constsym, varsym, procsym, writesym, readsym, elsesym
} token_type;

typedef enum{comment = 0, longword = -1, wrongstart = -2, invaldsym = -3, endOfFile = -4 } errors;

typedef enum{false, true} bool;


typedef enum
{
  LIT = 1, OPR, LOD, STO, CAL, INC, JMP, JPC, SIO
} opCodes;

typedef enum
{
  OPR_NEG = 1, OPR_ADD, OPR_SUB, OPR_MUL, OPR_DIV, OPR_ODD, OPR_MOD, OPR_EQL, OPR_NEQ, OPR_LSS, OPR_LEQ, OPR_GTR, OPR_GEQ
} oprCodes;

typedef enum
{
  undefinedError = 1, missingPeriod, missingIdent, invalidName, missingEquals,
  missingNumber, missingSemiColon, missingEnd, missingBecomes, missingThen,
  missingDo, notRelation, missingParen, longName, invalidCharInName, tooManySymbols,
  identMissing, codeTooLong, notVarOrConst, tooManyLexiLevels
} parserErrors;

typedef enum
{
    constant = 1, variable, procedure
} symboltypes;

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
//Parser
void parser();
void program();
void block();
void statement();
void condition();
void expression();
void term();
void factor();
//Symbol Creation
void createConstant(char*, int);
void createVariable(char*, int, int);
void createProcedure(char*, int, int);
void createSymbol(int, char*, int, int, int);
void deleteSymbol();
//Code Gen
int emit(int, int, int);
int find(char*);
int symboltype(int);
int symbollevel(int);
int symboladdress(int);
//Functional
void parserError(parserErrors);
void getToken();
void checkToken(token_type, parserErrors);
void checkForRelation();
void printSymbol(symbol*, int);
void printCode(int);

//PMZeroZed
void printFile(char*);


//PMZeroAlpha
int stack[MAX_STACK_HEIGHT];
instruction *codeScanner[MAX_CODE_LENGTH];
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

//PMSZeroBeta
char whitespace[] = {' ', '\t', '\v'};
char newlines[] = {'\n', '\r'};
char word[MAX_VAR_LEN];
char numb[MAX_NUM_LEN];
int lineNumber = 0;

//PMPZeroGamma
codeLine *codeParser[MAX_CODE_LENGTH];
int cx = 0;
symbol *symbolTable[MAX_TABLE_SIZE];//stores all the symbols!
int tp = 0;//points to the top of the table (i.e. the current symbol)
int token;//current token
int L = -1;
int M[MAX_LEXI_LEVEL];
char word[MAX_VAR_LEN];//stores any inputed variable names.
int number;//same as above, except it is a number!
int tokenNumber = -1;
FILE* inputFile;
FILE* outputCodeFile;
FILE* outputTableFilePretty;
FILE* outputTableFile;

//Print to Console Toggles
bool printT = false;//Token
bool printS = false;//SymbolTable
bool printM = false;//Machine Code
bool printA = false;//disassembled code
bool printV = false;//stack trace

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
    if(result != 0) return result;

    //PMPZeroGamma
    parser();
    if(printS) printFile("symboltable.txt");
    if(printM) printFile("mcode.txt");


    //Initialize all cells of the stack store to 0
    nullArray(MAX_STACK_HEIGHT, stack, 0);
    //set all of the instructions to null (explicit setting)
    for(i = 0; i < MAX_CODE_LENGTH; i++)
        codeScanner[i] = NULL;
    //currently, all of the possible stacks are nonexistant
    //As such, they don't start yet!
    nullArray(MAX_LEXI_LEVEL, startOfStacks, 0);
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
    exit_status = exitCheck(outputStackTrace, exit_status);
    fclose(outputStackTrace);
    fclose(outputACode);

    if(printA) printFile("acode.txt");
    if(printV) printFile("stacktrace.txt");
    return exit_status;
}

int loadIR(FILE* output)
{
    int i, exit_status = 0;
    bool error = false;
    for(i = 0; i < CODE_LENGTH; i++){
        ir = codeScanner[i];

        fprintf(output, "%-3d    %s(%02d) %d %d\n", i, getOP(ir->op), ir->op, ir->l, ir->m);


        error = !checkIsValid(output, ir);

        if(error) exit_status = 4;
    }
    return exit_status;
}

int fetch(FILE* output)
{
    //FETCH
    ir = codeScanner[pc];
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
            fprintf(output, "    MAXIMUM AMOUNT OF LEXIS = %d\n", MAX_LEXI_LEVEL);
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
        codeScanner[eggs] = temp;
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
                if(in->l > MAX_LEXI_LEVEL){

                    lexiError(output, in, "not greater than ");
                    fprintf(output, "%d\n", MAX_LEXI_LEVEL);
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
    while(codeScanner[i] != NULL)
    {
        printf("%d %d %d\n", codeScanner[i]->op, codeScanner[i]->l, codeScanner[i]->m);
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

//PMPZeroGamma
void parser()
{
    inputFile = fopen("tokenlist.txt", "r");
    outputCodeFile = fopen("mcode.txt", "w");
    outputTableFilePretty = fopen("aestheticsymboltable.txt", "w");
    outputTableFile = fopen("symboltable.txt", "w");
    fprintf(outputTableFile, "%-13s Type  Level Value\n", "Name");

    int i;
    program();
	//for(i = 0; i < 5; i++)
	//{
	//	printSymbol(symbolTable[i],i);
	//}
    for(i = 0; i < MAX_CODE_LENGTH; i++)
    {
        if(codeParser[i] == NULL)
            break;
        printCode(i);
    }
    fclose(inputFile);
    fclose(outputCodeFile);
    fclose(outputTableFilePretty);
    fclose(outputTableFile);
}

void program()
{
  getToken();
  block();
  if(token != periodsym)
  {
    parserError(missingPeriod);
  }
}

void block()
{
  L++;
  if(L > MAX_LEXI_LEVEL)
    parserError(tooManyLexiLevels);
  M[L] = 4;
  int jmpAddr = emit(JMP, 0, 0);
  if(token == constsym)
  {
    do
    {
      getToken();
      checkToken(identsym, missingIdent);
      getToken();
      checkToken(eqsym, missingEquals);
      getToken();
      checkToken(numbersym, missingNumber);
      getToken();
      createConstant(word, number);//NEEDS CHECKING
    } while(token == commasym);
    checkToken(semicolonsym, missingSemiColon);
    getToken();
  }
  if(token == varsym)
  {
    do
    {
      getToken();
      checkToken(identsym, missingIdent);
      getToken();
      createVariable(word, L, M[L]);//NEEDS CHECKING
      M[L]++;//NEEDS CHECKING
    } while(token == commasym);
    //emit(INC, 0, M[L]);
    checkToken(semicolonsym, missingSemiColon);
    getToken();
  }
  while(token == procsym)
  {
    getToken();
    checkToken(identsym, missingIdent);
    getToken();
    checkToken(semicolonsym, missingSemiColon);
    getToken();
    //M[L] = 4;//NEEDS CHECKING
    createProcedure(word, L, 1);//NEEDS CHECKING
    block();
    //emit(SIO, 0, 1);
    checkToken(semicolonsym, missingSemiColon);
    getToken();
  }
  codeParser[jmpAddr]->m = cx;
  emit(INC, 0, M[L]);
  statement();
  L--;
  if(L < 0)
    emit(SIO + 2, 0, 3);
  else
    emit(OPR, 0, 0);
  deleteSymbol();
}

void statement()
{
  int cx1, cx2;
  int i;

  if(token == identsym)
  {
      //printf("%s",word);
    i = find(word);
    if(i == -1)
    {
      parserError(identMissing);
    }
    if(symboltype(i) != variable)
    {
      parserError(identMissing);
    }

    getToken();
    checkToken(becomessym, missingBecomes);
    getToken();
    expression();
    emit(STO, L - symbollevel(i), symboladdress(i));
  }
  else if(token == callsym)//coded
  {
    getToken();
    checkToken(identsym, missingIdent);
    i = find(word);
    if(i == -1)
        parserError(identMissing);
    if(symboltype(i) != procedure)
        parserError(identMissing);
    emit(CAL, L - symbollevel(i), symboladdress(i));
    getToken();
  }
  else if(token == beginsym)
  {
    getToken();
    statement();
    while(token == semicolonsym)
    {
      getToken();
      statement();
    }
    checkToken(endsym, missingEnd);
    getToken();
  }
  else if(token == ifsym)//coded
  {
    getToken();
    condition();
    checkToken(thensym, missingThen);
    getToken();
    cx1 = cx;
    emit(JPC, 0, 0);
    statement();
    cx2 = cx;
    //CHECK!!!
    emit(JMP, 0, 0);
    codeParser[cx1]->m = cx;
    if(token == elsesym)
    {
        getToken();
        statement();
    }
    codeParser[cx2]->m = cx;
    //test?
    //printf("\nJUMP IF+++++%d\n",cx);
  }
  else if(token == whilesym)//coded
  {
    cx1 = cx;
    getToken();
    condition();
    cx2 = cx;
    emit(JPC, 0, 0);
    checkToken(dosym, missingDo);
    getToken();
    statement();
    //printf("\nJUMP WHILE=====%d,%d\n", cx1, cx);
    emit(JMP, 0, cx1);
    codeParser[cx2]->m = cx;
  }
  else if(token == readsym)
  {
      getToken();
      checkToken(identsym,identMissing);
      i = find(word);
      if(i == -1 || symboltype(i) != variable)
        parserError(missingIdent);
      emit(SIO + 1, 0, 2);
      emit(STO, L - symbollevel(i), symboladdress(i));
  }
  else if(token == writesym)
  {
      getToken();
      checkToken(identsym,identMissing);
      i = find(word);
      if(i == -1)
        parserError(missingIdent);
      if(symboltype(i) == procedure)
        parserError(notVarOrConst);
      if(symboltype(i) == variable)
        emit(LOD, L - symbollevel(i), symboladdress(i));
      else
        emit(LIT, 0, symbolTable[i]->val);
      emit(SIO, 0, 1);
  }
}

void condition()//coded
{
    int code;
  if(token == oddsym)
  {
   // printf("\n$%d$\n",OPR_ODD);

    getToken();
    expression();
    emit(OPR, 0, OPR_ODD);
  }
  else
  {
    expression();
    checkForRelation();
    switch(token)
        {
        case(eqsym):
            code = OPR_EQL;
            break;
        case(neqsym):
            code = OPR_NEQ;
            break;
        case(lessym):
            code = OPR_LSS;
            break;
        case(leqsym):
            code = OPR_LEQ;
            break;
        case(gtrsym):
            code = OPR_GTR;
            break;
        case(geqsym):
            code = OPR_GEQ;
            break;
        }
    getToken();
    expression();
    emit(OPR, 0, code);
  }
}

void expression()//coded
{
  int addop;
  if(token == plussym || token == minussym)
  {
    addop = token;
    getToken();
    term();
    if(addop == minussym)
    {
      emit(OPR, 0, OPR_NEG);
    }
  }
  else
  {
    term();
  }
  while(token == plussym || token == minussym)
  {
    addop = token;
    getToken();
    term();
    if(addop == plussym)
    {
      emit(OPR, 0, OPR_ADD);
    }
    else
    {
      emit(OPR, 0, OPR_SUB);
    }
  }
}

void term()//coded
{
  int mulop;
  factor();
  while(token == multsym || token == slashsym)
  {
    mulop = token;
    getToken();
    factor();
    if(mulop == multsym)
    {
      emit(OPR, 0, OPR_MUL);
    }
    else
    {
      emit(OPR, 0, OPR_DIV);
    }
  }
}

void factor()//coded I believe...
{
  int i;

  if(token == identsym)
  {
    //printf("%s\n",word);
    i = find(word);
    //printf("%d\n", i);
    if(i == -1)
      parserError(identMissing);
    else if(symboltype(i) == constant)
      emit(LIT, 0, symbolTable[i]->val);
    else if(symboltype(i) == variable)
      emit(LOD, L - symbollevel(i), symboladdress(i));
    getToken();
  }
  else if(token == numbersym)
  {
    emit(LIT, 0, number);
    getToken();
  }
  else if(token == lparentsym)
  {
    getToken();
    expression();
    checkToken(rparentsym, missingParen);
    getToken();
  }
  else
    parserError(undefinedError);
}

void createConstant(char *name, int val)
{
  createSymbol(1, name, val, (int)NULL, (int)NULL);
}

void createVariable(char *name, int level, int addr)
{
  createSymbol(2, name, (int)NULL, level, addr);
}

void createProcedure(char *name, int level, int addr)
{
  createSymbol(3, name, (int)NULL, level, addr);
}

void createSymbol(int kind, char *name, int val, int level, int addr)
{
  symbol *temp = (symbol*)malloc(sizeof(symbol));
  temp->kind = kind;
  strcpy(temp->name, name);
  temp->val = val;
  temp->level = level;
  temp->addr = addr;
  printSymbol(temp, tp);
  symbolTable[tp] = temp;
  tp++;
}

void deleteSymbol()//untested
{
    //TODO: FIX!!!
    symbol *temp;
    while(tp > 0)
    {
        temp = symbolTable[tp - 1];
        if(temp->level > L)
        {
            free(temp);
            tp--;
        }
        else
        {
            break;
        }

    }
}

int emit(int op, int l, int m)
{
  if(cx >= MAX_CODE_LENGTH)
    parserError(codeTooLong);
  else
  {
    codeLine *temp = (codeLine*)malloc(sizeof(codeLine));
    codeParser[cx] = temp;
    codeParser[cx]->op = op;
    codeParser[cx]->l = l;
    codeParser[cx]->m = m;
    cx++;
  }
  return cx-1;
}

int find(char* name)
{
  int i, len;
  symbol *temp;
  for(len = 0; len < MAX_TABLE_SIZE; len++)
  {
    if(symbolTable[len] == NULL)
        break;
  }
  for(i = len - 1; i >= 0; i--)
  {
    temp = symbolTable[i];
    if(strcmp(temp->name, name) == 0)
      return i;
  }
  return -1;
}

int symboltype(int i)
{
  return symbolTable[i]->kind;
}

int symbollevel(int i)
{
  return symbolTable[i]->level;
}

int symboladdress(int i)
{
  return symbolTable[i]->addr;
}

void parserError(parserErrors err)
{
  printf("\n==================\n");
  printf("ERROR: ");
  switch(err)
  {
    default:
    case(undefinedError):
      printf("Something went wrong, but the appropriate error handler has not been implemented.");
      break;
    case(missingPeriod):
      printf("Expected a period ('.') after final 'end' keyword.");
      break;
    case(missingIdent):
      printf("Expected a variable name.");
      break;
    case(invalidName):
      printf("Variable names must start with a letter.");
      break;
    case(missingEquals):
      printf("Expected the equals sign ('=').");
      break;
    case(missingNumber):
      printf("Expected a value. Either a literal or variable would do.");
      break;
    case(missingSemiColon):
      printf("Expected a semicolon. Keep in mind that all statements MUST end with a semicolon!");
      break;
    case(missingEnd):
      printf("Expected the keyword 'end'.");
      break;
    case(missingBecomes):
      printf("Expected an assignment opperator (':=').");
      break;
    case(missingThen):
      printf("Expected the keyword 'then'.");
      break;
    case(missingDo):
      printf("Expected the keyword 'do'.");
      break;
    case(notRelation):
      printf("Expected a relation (e.g. '=', '<>', '<', '<=', '>', '>=').");
      break;
    case(missingParen):
      printf("Expected a right parenthesis (')').");
      break;
    case(longName):
      printf("Variable name must be 11 chars or less! (Not counting the termination character ('\\0')");
      break;
    case(invalidCharInName):
      printf("Variable names must be made up of Alphanumerical characters only. ('A-F' and '0-9')");
      break;
    case(identMissing):
        printf("Invalid variable");
        break;
    case(tooManySymbols):
        printf("Too much memory has been used in the Symbol Table.");
        break;
    case(codeTooLong):
        printf("Too much memory has been used in the Code Generator.");
        break;
    case(notVarOrConst):
        printf("Must be either a constant or variable, not a number or procedure.");
        break;
    case(tooManyLexiLevels):
        printf("This code has to many Lexi Levels. The Maximum allowed is %d.", MAX_LEXI_LEVEL);
  }
  printf("\nToken #: %d", tokenNumber);
  exit(err);
}

void getToken()
{
  char c;
  int i = 1;
  //scanf("%d", &token);
  fscanf(inputFile, "%d", &token);
  if(token == identsym)
  {
    //gotta get past the whitespace first...
    //getchar();
    getc(inputFile);
    c = getc(inputFile);
    //c = getchar();
    word[0] = c;
    if(!isalpha(word[0]))
      parserError(invalidName);
    while(c != ' ')
    {
      //c = getchar();
      c = getc(inputFile);
      //DOUBLE CHECK THIS ERROR CHECK WORKS AS EXPECTED!
      if(!isalnum(c) && c != ' ')
         parserError(invalidCharInName);
      word[i] = c;
      i++;
      if(i == 12)
        parserError(longName);
    }
    word[i] = '\0';
  }
  else if(token == numbersym)
    //scanf("%d", &number);
    fscanf(inputFile, "%d", &number);

  tokenNumber++;
}

void checkToken(token_type givenType, parserErrors reportableError)
{
  if(token != givenType)
    parserError(reportableError);
}

void checkForRelation()
{
  switch(token)
  {
    case(eqsym):
    case(neqsym):
    case(lessym):
    case(leqsym):
    case(gtrsym):
    case(geqsym):
      break;
default:
      parserError(notRelation);
  }
}

void printSymbol(symbol *in, int number)
{
  char* spam = "";
  int eggs = 0;
  //currently configed for console output. Easy to adapt to file output.
  fprintf(outputTableFilePretty,"\nSymbol #%d\n==================\n", number);
  fprintf(outputTableFilePretty,"Kind:  ");
  switch(in->kind)
  {
    case(1):
      fprintf(outputTableFilePretty,"CONSTANT");
      spam = "const";
      eggs = in->val;
      break;
    case(2):
      fprintf(outputTableFilePretty,"VARIABLE");
      spam = "var";
      eggs = in->addr;
      break;
    case(3):
      fprintf(outputTableFilePretty,"PROCEDURE");
      spam = "proc";
      eggs = in->addr;
      break;
  }
  fprintf(outputTableFilePretty,"\nName: %s", in->name);
  fprintf(outputTableFilePretty,"\nValue: %d\nL:     %d\nM:     %d\n", in->val, in->level, in->addr);
  fprintf(outputTableFilePretty,"==================\n");

  fprintf(outputTableFile, "%-13s %-5s %-5d %-5d\n", in->name, spam, in->level, eggs);
}

void printCode(int i)
{
  codeLine *in = codeParser[i];
  fprintf(outputCodeFile,"%d %d %d\n", in->op, in->l, in->m);
}



//PMZeroZed
void printFile(char* filename)
{
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Contents of \"%s\"", filename);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    FILE *file;
    char c;
    file = fopen(filename, "r");

    c = fgetc(file);
    while(c != EOF)
    {
        printf("%c", c);
        c = getc(file);
    }

    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    fclose(file);
}
