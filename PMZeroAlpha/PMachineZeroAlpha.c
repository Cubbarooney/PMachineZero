/*PM/0 Build Alpha
* Jacob "Cubby" Rubenstein
* 09/12/15
* For System Software with Matthew Gerber
* Assignment 1
*/

#include <stdio.h>
#include <stdlib.h>

typedef struct{
    int op;
    int l;
    int m;
} instruction;
typedef enum{false, true} bool;

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

#define MAX_STACK_HEIGHT 2000
#define MAX_CODE_LENGTH 500
#define MAX_LEXI_LEVELS 3

int stack[MAX_STACK_HEIGHT];
instruction *code[MAX_CODE_LENGTH];
//TODO Change the way this is implemented. The bps are all stored in the stack!
int startOfStacks[MAX_LEXI_LEVELS];

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

int main(void){
    int i;
    int exit_status = 0;
    FILE *output;

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
    output = fopen("stacktrace.txt", "w");
    fprintf(output, "Line   OP      L M\n");

    //create the IR and print each instruction to the output file
    exit_status = loadIR(output);

    //place the titles for the stacktrace now that we have loaded the program.
    if(exit_status == 0){
        fprintf(output, "\nLine   OP      L M     PC  BP  SP   Stack\n");
        fprintf(output,   "Initial Values   -->   %-3d %d   %-3d   ", pc, bp, sp);
        outputStack(output);
        printLine(output);
    }

    //CORE
    while(exit_status == 0){
        exit_status = fetch(output);
        //SEVERE ERROR, END IMEDIATELY!
        if(exit_status == 2) break;
        exit_status = execute(output);

    }

    return exitCheck(output, exit_status);
}

int loadIR(FILE* output){
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

int fetch(FILE* output){
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
            if(numOfLexis > MAX_LEXI_LEVELS){
                return 3;
            }
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
            printf("%d\n", pop());
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
    fclose(output);
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
                if(in->l > MAX_LEXI_LEVELS){

                    lexiError(output, in, "not greater than ");
                    fprintf(output, "%d\n", MAX_LEXI_LEVELS);
                    flag = false;
                }
                break;
            case(5):
                if(in->l > MAX_LEXI_LEVELS){

                    lexiError(output, in, "not greater than ");
                    fprintf(output, "%d\n", MAX_LEXI_LEVELS);
                    flag = false;
                }
                if(code[in->m]->op != 6){
                    fprintf(output, "ERROR: This statement does not jump to a INC instruction\n");
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
