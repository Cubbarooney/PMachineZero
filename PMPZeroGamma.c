#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef enum
{
  nulsym = 1, identsym, numbersym, plussym, minussym, multsym,  slashsym,    //|
  oddsym, eqsym, neqsym, lessym, leqsym, gtrsym, geqsym, lparentsym, rparentsym,
  commasym, semicolonsym, periodsym, becomessym, beginsym, endsym, ifsym,
  thensym,
  whilesym, dosym, callsym, constsym, varsym, procsym, writesym,
  readsym, elsesym
} token_type;

typedef enum//todo: fix SIO
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
  identMissing, codeTooLong
} parserErrors;

typedef enum
{
    constant = 1, variable, procedure
} symboltypes;

#define MAX_TABLE_SIZE 100
#define MAX_CODE_LENGTH 500
#define MAX_WORD_LENGTH 12 //11 chars plus '\0'
#define MAX_LEXI_LEVEL 3

/*For Constants, store kind, name, and val
  For variables, store kind, name, L, and M
  For procedures, store kind, name, L, and M*/

typedef struct symbol
{
  int kind;	//const = 1, var = 2, proc = 3
  char name[MAX_WORD_LENGTH];//name is at most 11 chars long
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

//Parser

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

codeLine *code[MAX_CODE_LENGTH];
int cx = 0;
symbol *symbolTable[MAX_TABLE_SIZE];//stores all the symbols!
int tp = 0;//points to the top of the table (i.e. the current symbol)
int token;//current token
int L = -1;
int M[MAX_LEXI_LEVEL];
char word[MAX_WORD_LENGTH];//stores any inputed variable names.
int numb;//same as above, except it is a number!
int tokenNumber = -1;
FILE* inputFile;
FILE* outputCodeFile;
FILE* outputTableFile;

int main()
{
    //M[0] = 4;//indexed at 1, so the fifth item is at five (funny how that works)

  inputFile = fopen("tokenlist.txt", "r");
//  outputCodeFile = fopen("symboltable.txt", "w");
//  outputTableFile = fopen("mcode.txt", "w");
	int i;
	program();
	//for(i = 0; i < 5; i++)
	//{
	//	printSymbol(symbolTable[i],i);
	//}
	for(i = 0; i < MAX_CODE_LENGTH; i++)
	{
		if(code[i] == NULL)
			break;
		printCode(i);
	}
  fclose(inputFile);
//  fclose(outputCodeFile);
//  fclose(outputTableFile);
  return 0;
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
      createConstant(word, numb);//NEEDS CHECKING
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
      printf(", %d++ ,",M[L]);
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
  code[jmpAddr]->m = cx;
  printf("\n*%d*L:%d\n",M[L],L);
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
    printf(",%d\n",token);
    emit(STO, L - symbollevel(i), symboladdress(i));
  }
  else if(token == callsym)//coded
  {
    getToken();
    checkToken(identsym, missingIdent);
    i = find(word);
    if(i == -1)
        parserError(-200);
    if(symboltype(i) != procedure)
        parserError(-210);
    emit(CAL, L - symbollevel(i), symboladdress(i));
    getToken();
  }
  else if(token == beginsym)
  {
    printf("\nENTERING BEGINSYM\n");
    getToken();
    statement();
    while(token == semicolonsym)
    {
      getToken();
      printf("-%d\n",token);
      statement();
      printf("=%d\n",token);
    }
    printf("?%d\n",token);
    checkToken(endsym, missingEnd);
    getToken();
    printf("+%d\n",token);
    printf("\nLEAVING BEGINSYM\n");
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

    emit(JMP, 0, cx + 1);
    code[cx1]->m = cx;
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
    code[cx2]->m = cx;
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
  printf(".%d\n",token);
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
    printf("\n %d\n",token);
  }
  else if(token == numbersym)
  {
    emit(LIT, 0, numb);
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
  tp--;
  symbol *temp = symbolTable[tp];
  while(temp->level > L && tp > 0)
  {
      tp--;
      temp = symbolTable[tp];
  }
  tp++;
}

int emit(int op, int l, int m)
{
  if(cx >= MAX_CODE_LENGTH)
    parserError(codeTooLong);
  else
  {
    codeLine *temp = (codeLine*)malloc(sizeof(codeLine));
    code[cx] = temp;
    code[cx]->op = op;
    code[cx]->l = l;
    code[cx]->m = m;
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
  printf("TOKEN: %d, ",token);
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
    //scanf("%d", &numb);
    fscanf(inputFile, "%d", &numb);

  tokenNumber++;
  printf("NUMBER: %d\n", tokenNumber);
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
  //currently configed for console output. Easy to adapt to file output.
  printf("\nSymbol #%d\n==================\n", number);
  printf("Kind:  ");
  switch(in->kind)
  {
    case(1):
      printf("CONSTANT");
      break;
    case(2):
      printf("VARIABLE");
      break;
    case(3):
      printf("PROCEDURE");
      break;
  }
  printf("\nName: %s", in->name);
  printf("\nValue: %d\nL:     %d\nM:     %d\n", in->val, in->level, in->addr);
  printf("\n==================\n");
}

void printCode(int i)
{
  codeLine *in = code[i];
  printf("%d %d %d\n", in->op, in->l, in->m);
}
