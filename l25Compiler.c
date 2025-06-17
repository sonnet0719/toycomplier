/*
 * l25 complier program implemented in C
 * PL/0编译器（完整版本）
 * The program has been tested on Visual Studio 2022
 *
 * 使用方法：
 * 运行后输入l25源程序文件名
 * 回答是否输出虚拟机代码
 * 回答是否输出符号表
 * fcode.txt输出虚拟机代码
 * foutput.txt输出源文件、出错示意（如有错）和各行对应的生成代码首地址（如无错）
 * fresult.txt输出运行结果
 * ftable.txt输出符号表
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define bool int
#define true 1
#define false 0

#define norw 12			 /* 保留字个数 */
#define txmax 100		 /* 符号表容量 */
#define nmax 14			 /* 数字的最大位数 */
#define al 10			 /* 标识符的最大长度 */
#define maxerr 30		 /* 允许的最多错误数 */
#define amax 0xfffffffff /* 地址上界*/
#define cxmax 200		 /* 最多的虚拟机代码数 */
#define stacksize 500	 /* 运行时数据栈元素最多为500个 */

/* 符号 */
enum symbol
{
	nul,	   // 空
	ident,	   // 标识符
	number,	   // 数字
	plus,	   // 加法 +
	minus,	   // 减法 -
	times,	   // 乘法 *
	slash,	   // 除法 /
	eql,	   // 等于 ==
	neq,	   // 不等于 !=
	lss,	   // 小于 <
	leq,	   // 小于等于 <=
	gtr,	   // 大于 >
	geq,	   // 大于等于 >=
	lparen,	   // 左括号 (
	rparen,	   // 右括号 )
	lbrace,	   // 左大括号 {
	rbrace,	   // 右大括号 }
	comma,	   // 逗号 ,
	semicolon, // 分号 ;
	period,	   // 结束符 .
	becomes,   // 赋值 =
	elsesym,   // 否则 "else"
	funcsym,   // 函数 "function"
	ifsym,	   // 判断 "if"
	inputsym,  // 输入 "input"
	letsym,	   // 声明 "let"
	mainsym,   // 函数 "main"
	outputsym, // 输出 "output"
	progsym,   // 程序 "program"
	returnsym, // 返回 "return"
	whilesym,  // 循环 "while"
	trysym,	   // 异常处理 "try"
	catchsym   // 异常处理 "catch"
};
#define symnum 33

/* 符号表中的类型 */
enum object
{
	variable,
	param,
	function,
};

/* 虚拟机代码指令 */
enum fct
{
	lit,
	opr,
	lod,
	sto,
	cal,
	ini,
	jmp,
	jpc,
};
#define fctnum 8

/* 虚拟机代码结构 */
struct instruction
{
	enum fct f; /* 虚拟机代码指令 */
	int a;		/* 根据f的不同而不同 */
};

bool listswitch;  /* 显示虚拟机代码与否 */
bool tableswitch; /* 显示符号表与否 */
char ch;		  /* 存放当前读取的字符，getch 使用 */
enum symbol sym;  /* 当前的符号 */
char id[al + 1];  /* 当前ident，多出的一个字节用于存放0 */
int num;		  /* 当前number */
int cc, ll;		  /* getch使用的计数器，cc表示当前字符(ch)的位置 */
int cx;			  /* 虚拟机代码指针, 取值范围[0, cxmax-1]*/
int tx;			  /* 当前符号表尾，0 表示仅有哨兵 */
int dx;
int curFuncIdx = -1;
char line[81];					/* 读取行缓冲区 */
char a[al + 1];					/* 临时符号，多出的一个字节用于存放0 */
struct instruction code[cxmax]; /* 存放虚拟机代码的数组 */
char word[norw][al];			/* 保留字 */
enum symbol wsym[norw];			/* 保留字对应的符号值 */
enum symbol ssym[256];			/* 单字符的符号值 */
char mnemonic[fctnum][5];		/* 虚拟机代码指令名称 */
bool declbegsys[symnum];		/* 表示声明开始的符号集合 */
bool statbegsys[symnum];		/* 表示语句开始的符号集合 */
bool facbegsys[symnum];			/* 表示因子开始的符号集合 */

/* 符号表结构 */
struct tablestruct
{
	char name[al];	  /* 名字 */
	enum object kind; /* 类型：const，var或procedure */
	int adr;		  /* 地址，仅const不使用 */
	int size;		  /* 需要分配的数据区空间, 仅procedure使用 */
	int paramCnt;	  /* 函数形参个数           */
	unsigned attr;	  /* 位标志：bit0=isParam…  */
};

struct tablestruct table[txmax]; /* 符号表 */

FILE *fin;	   /* 输入源文件 */
FILE *ftable;  /* 输出符号表 */
FILE *fcode;   /* 输出虚拟机代码 */
FILE *foutput; /* 输出文件及出错示意（如有错）、各行对应的生成代码首地址（如无错） */
FILE *fresult; /* 输出执行结果 */
char fname[al];
int err; /* 错误计数器 */

void error(int n);
void getsym();
void getch();
void init();
void gen(enum fct x, int z);
void test(bool *s1, bool *s2, int n);
int inset(int e, bool *s);
int addset(bool *sr, bool *s1, bool *s2, int n);
int subset(bool *sr, bool *s1, bool *s2, int n);
int mulset(bool *sr, bool *s1, bool *s2, int n);
void program(bool *fsys); /* 顶层 */
void parse_function_header(bool *fsys);
void block(int *ptx, bool *fsys, int isFunc, int *retParamCnt);
void interpret();
void factor(bool *fsys, int *ptx);
void term(bool *fsys, int *ptx);
void condition(bool *fsys, int *ptx);
void expression(bool *fsys, int *ptx);
void call_handle(int pos);
void statement(bool *fsys, int *ptx, int *pdx);
void listcode(int cx0);
void listall();
int position(char *idt, int tx);
int enter(enum object k, int *ptx, int *pdx);

/* 主程序开始 */
int main()
{
	bool nxtlev[symnum];

	printf("Input l25 file?   ");
	scanf("%s", fname); /* 输入文件名 */

	if ((fin = fopen(fname, "r")) == NULL)
	{
		printf("Can't open the input file!\n");
		exit(1);
	}

	ch = fgetc(fin);
	if (ch == EOF)
	{
		printf("The input file is empty!\n");
		fclose(fin);
		exit(1);
	}
	rewind(fin);

	if ((foutput = fopen("foutput.txt", "w")) == NULL)
	{
		printf("Can't open the output file!\n");
		exit(1);
	}

	if ((ftable = fopen("ftable.txt", "w")) == NULL)
	{
		printf("Can't open ftable.txt file!\n");
		exit(1);
	}

	printf("List object codes?(Y/N)"); /* 是否输出虚拟机代码 */
	scanf("%s", fname);
	listswitch = (fname[0] == 'y' || fname[0] == 'Y');

	printf("List symbol table?(Y/N)"); /* 是否输出符号表 */
	scanf("%s", fname);
	tableswitch = (fname[0] == 'y' || fname[0] == 'Y');

	init(); /* 初始化 */
	err = 0;
	cc = ll = cx = 0;
	ch = ' ';
	tx = 0;
	cx = 0; /* 代码计数器 */

	getsym();

	addset(nxtlev, declbegsys, statbegsys, symnum);
	program(nxtlev); /* ← 取代原 block(...) */

	if (err == 0)
	{
		printf("\n===Parsing success!===\n");
		fprintf(foutput, "\n===Parsing success!===\n");

		if ((fcode = fopen("fcode.txt", "w")) == NULL)
		{
			printf("Can't open fcode.txt file!\n");
			exit(1);
		}

		if ((fresult = fopen("fresult.txt", "w")) == NULL)
		{
			printf("Can't open fresult.txt file!\n");
			exit(1);
		}

		listall(); /* 输出所有代码 */
		fclose(fcode);

		interpret(); /* 调用解释执行程序 */
		fclose(fresult);
	}
	else
	{
		printf("\n%d errors in l25 program!\n", err);
		fprintf(foutput, "\n%d errors in l25 program!\n", err);
	}

	fclose(ftable);
	fclose(foutput);
	fclose(fin);

	return 0;
}

/*
 * 初始化
 */
void init()
{
	int i;

	/* 设置单字符符号 */
	for (i = 0; i <= 255; i++)
	{
		ssym[i] = nul;
	}
	ssym['+'] = plus;
	ssym['-'] = minus;
	ssym['*'] = times;
	ssym['/'] = slash;
	ssym['('] = lparen;
	ssym[')'] = rparen;
	ssym['{'] = lbrace;
	ssym['}'] = rbrace;
	ssym[','] = comma;
	ssym['.'] = period;
	ssym[';'] = semicolon;

	/* 设置保留字名字,按照字母顺序，便于二分查找 */
	strcpy(word[0], "catch");
	wsym[0] = catchsym;
	strcpy(word[1], "else");
	wsym[1] = elsesym;
	strcpy(word[2], "func");
	wsym[2] = funcsym;
	strcpy(word[3], "if");
	wsym[3] = ifsym;
	strcpy(word[4], "input");
	wsym[4] = inputsym;
	strcpy(word[5], "let");
	wsym[5] = letsym;
	strcpy(word[6], "main");
	wsym[6] = mainsym;
	strcpy(word[7], "output");
	wsym[7] = outputsym;
	strcpy(word[8], "program");
	wsym[8] = progsym;
	strcpy(word[9], "return");
	wsym[9] = returnsym;
	strcpy(word[10], "try");
	wsym[10] = trysym;
	strcpy(word[11], "while");
	wsym[11] = whilesym;

	/* 设置指令名称 */
	strcpy(&(mnemonic[lit][0]), "lit");
	strcpy(&(mnemonic[opr][0]), "opr");
	strcpy(&(mnemonic[lod][0]), "lod");
	strcpy(&(mnemonic[sto][0]), "sto");
	strcpy(&(mnemonic[cal][0]), "cal");
	strcpy(&(mnemonic[ini][0]), "int");
	strcpy(&(mnemonic[jmp][0]), "jmp");
	strcpy(&(mnemonic[jpc][0]), "jpc");

	/* 设置符号集 */
	for (i = 0; i < symnum; i++)
	{
		declbegsys[i] = false;
		statbegsys[i] = false;
		facbegsys[i] = false;
	}

	/* 设置声明开始符号集 */
	declbegsys[funcsym] = true;

	/* 设置语句开始符号集 */
	statbegsys[inputsym] = true;
	statbegsys[outputsym] = true;
	statbegsys[ifsym] = true;
	statbegsys[whilesym] = true;
	statbegsys[letsym] = true;
	statbegsys[ident] = true;
	statbegsys[trysym] = true;

	/* 设置因子开始符号集 */
	facbegsys[ident] = true;
	facbegsys[number] = true;
	facbegsys[lparen] = true;
}

/*
 * 用数组实现集合的集合运算
 */
int inset(int e, bool *s)
{
	return s[e];
}

int addset(bool *sr, bool *s1, bool *s2, int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		sr[i] = s1[i] || s2[i];
	}
	return 0;
}

int subset(bool *sr, bool *s1, bool *s2, int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		sr[i] = s1[i] && (!s2[i]);
	}
	return 0;
}

int mulset(bool *sr, bool *s1, bool *s2, int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		sr[i] = s1[i] && s2[i];
	}
	return 0;
}

/*
 *	出错处理，打印出错位置和错误编码
 */
void error(int n)
{
	char space[81];
	memset(space, 32, 81);

	space[cc - 1] = 0; /* 出错时当前符号已经读完，所以cc-1 */

	printf("**%s^%d\n", space, n);
	fprintf(foutput, "**%s^%d\n", space, n);

	err = err + 1;
	if (err > maxerr)
	{
		exit(1);
	}
}

/*
 * 过滤空格，读取一个字符
 * 每次读一行，存入line缓冲区，line被getsym取空后再读一行
 * 被函数getsym调用
 */
void getch()
{
	if (cc == ll) /* 判断缓冲区中是否有字符，若无字符，则读入下一行字符到缓冲区中 */
	{
		if (feof(fin))
		{
			ch = EOF; /* 交由 getsym() 把 sym 设成 nul */
			return;
		}
		ll = 0;
		cc = 0;
		printf("%d ", cx);
		fprintf(foutput, "%d ", cx);
		ch = ' ';
		while (ch != 10)
		{
			if (EOF == fscanf(fin, "%c", &ch))
			{
				line[ll] = 0;
				break;
			}

			printf("%c", ch);
			fprintf(foutput, "%c", ch);
			line[ll] = ch;
			ll++;
		}
	}
	ch = line[cc];
	cc++;
}

/*
 * 词法分析，获取一个符号
 */
void getsym()
{
	int i, j, k;

	while (ch == ' ' || ch == 10 || ch == 9) /* 过滤空格、换行和制表符 */
	{
		getch();
	}
	if (ch == EOF)
	{
		sym = nul;
		return;
	}
	if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) /* 当前的单词是标识符或是保留字 */
	{
		k = 0;
		do
		{
			if (k < al)
			{
				a[k] = ch;
				k++;
			}
			getch();
		} while ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'));
		a[k] = 0;
		strcpy(id, a);
		i = 0;
		j = norw - 1;
		do
		{ /* 搜索当前单词是否为保留字，使用二分法查找 */
			k = (i + j) / 2;
			if (strcmp(id, word[k]) <= 0)
			{
				j = k - 1;
			}
			if (strcmp(id, word[k]) >= 0)
			{
				i = k + 1;
			}
		} while (i <= j);
		if (i - 1 > j) /* 当前的单词是保留字 */
		{
			sym = wsym[k];
		}
		else /* 当前的单词是标识符 */
		{
			sym = ident;
		}
	}
	else
	{
		if (ch >= '0' && ch <= '9') /* 当前的单词是数字 */
		{
			k = 0;
			num = 0;
			sym = number;
			do
			{
				num = 10 * num + ch - '0';
				k++;
				getch();
				;
			} while (ch >= '0' && ch <= '9'); /* 获取数字的值 */
			k--;
			if (k > nmax) /* 数字位数太多 */
			{
				error(30);
			}
		}
		else
		{
			if (ch == '=') /* 检测赋值符号 */
			{
				getch();
				if (ch == '=')
				{
					sym = eql;
					getch();
				}
				else
				{
					sym = becomes; /* 赋值符号 */
				}
			}
			else
			{
				if (ch == '<') /* 检测小于或小于等于符号 */
				{
					getch();
					if (ch == '=')
					{
						sym = leq;
						getch();
					}
					else
					{
						sym = lss;
					}
				}
				else
				{
					if (ch == '>') /* 检测大于或大于等于符号 */
					{
						getch();
						if (ch == '=')
						{
							sym = geq;
							getch();
						}
						else
						{
							sym = gtr;
						}
					}
					else
					{
						if (ch == '!') /* 检测不等于符号 */
						{
							getch();
							if (ch == '=')
							{
								sym = neq;
								getch();
							}
							else
							{
								sym = nul;
								error(30);
							}
						}
						else
						{
							sym = ssym[ch]; /* 当符号不满足上述条件时，全部按照单字符符号处理 */
							if (sym != nul)
							{
								getch();
							}
						}
					}
				}
			}
		}
	}
}

/*
 * 生成虚拟机代码
 *
 * x: instruction.f;
 * z: instruction.a;
 */
void gen(enum fct x, int z)
{
	if (cx >= cxmax)
	{
		printf("Program is too long!\n"); /* 生成的虚拟机代码程序过长 */
		exit(1);
	}
	if (z >= amax)
	{
		printf("Displacement address is too big!\n"); /* 地址偏移越界 */
		exit(1);
	}
	code[cx].f = x;
	code[cx].a = z;
	cx++;
}

/*
 * 测试当前符号是否合法
 *
 * 在语法分析程序的入口和出口处调用测试函数test，
 * 检查当前单词进入和退出该语法单位的合法性
 *
 * s1:	需要的单词集合
 * s2:	如果不是需要的单词，在某一出错状态时，
 *      可恢复语法分析继续正常工作的补充单词符号集合
 * n:  	错误号
 */
void test(bool *s1, bool *s2, int n)
{
	if (!inset(sym, s1))
	{
		error(n);
		/* 当检测不通过时，不停获取符号，直到它属于需要的集合或补救的集合 */
		while ((!inset(sym, s1)) && (!inset(sym, s2)))
		{
			getsym();
		}
	}
}

/* <program> ::= program ident '{' { <func_def> } <main_block> '}' '.' */
void program(bool *fsys)
{
	/* program <ident> '{' {function-def} main-block '}' '.' */
	int cx0;
	dx = 3;

	if (sym != progsym)
		error(40);
	getsym();

	if (sym != ident)
		error(1); /* 程序名 */
	getsym();

	if (sym != lbrace)
		error(23);
	getsym();

	cx0 = cx;	 /* 保存当前 code 索引 */
	gen(jmp, 0); /* 先生成一条 jmp 占位，后面回填 */

	/* ---------- 0~多条 function 定义 ---------- */
	while (sym == funcsym)
	{
		bool funcFollow[symnum] = {0};
		funcFollow[funcsym] = funcFollow[mainsym] = funcFollow[rbrace] = funcFollow[semicolon] = true;
		parse_function_header(funcFollow); /* ↓ 见第 2 节 */
	}
	code[cx0].a = cx;

	/* ---------- main { stmt_list } ---------- */
	if (sym != mainsym)
		error(41);
	getsym();
	bool topFollow[symnum] = {0};
	topFollow[rbrace] = topFollow[period] = topFollow[semicolon] = true;
	block(
		/* ptx      */ &tx,
		/* fsys     */ topFollow, /* FOLLOW(program)，含 rbrace/period 即可 */
		/* isFunc   */ 0,		  /* ★ 不解析 return，不开返回槽 */
		/* retParamCnt */ NULL);

	if (sym != rbrace)
		error(24);
	getsym();
	if (sym == nul && feof(fin))
	{
		/* 什么也不做——正常结束 */}
		else
		{
			error(9);
		}
		if (tableswitch) /* 输出符号表 */
		{
			for (int i = 1; i <= tx; i++)
			{
				switch (table[i].kind)
				{
				case param:
					printf("    %d param %s ", i, table[i].name);
					printf("adr=%d\n", table[i].adr);
					fprintf(ftable, "    %d param %s ", i, table[i].name);
					fprintf(ftable, "adr=%d\n", table[i].adr);
					break;
				case variable:
					printf("    %d var   %s ", i, table[i].name);
					printf("addr=%d\n", table[i].adr);
					fprintf(ftable, "    %d var   %s ", i, table[i].name);
					fprintf(ftable, "ddr=%d\n", table[i].adr);
					break;
				case function:
					printf("    %d func  %s ", i, table[i].name);
					printf("addr=%d size=%d\n", table[i].adr, table[i].size);
					fprintf(ftable, "    %d func  %s ", i, table[i].name);
					fprintf(ftable, "addr=%d size=%d\n", table[i].adr, table[i].size);
					break;
				}
			}
		}
		printf("\n");
		fprintf(ftable, "\n");
}

/* 已读取到关键字 function */
void parse_function_header(bool *fsys)
{
	getsym(); /* 跳过 'function' */

	if (sym != ident)
		error(1);
	curFuncIdx = enter(function, &tx, &dx);
	getsym(); /* 跳过函数名 */

	table[curFuncIdx].adr = cx + 1;

	/* 保存进入本函数前的 dx 值 */
	int savedDx = dx;
	/* 重置 dx 为 3，让函数在自己的作用域内从 3 开始分配 SL/RA/ret-slot */
	dx = 3;

	int paramCnt = 0; /* ← 只在这里声明一次 */

	/* 直接把 '(' 及后续全部交给 block() */
	block(&tx, fsys, 1, /* isFunc = 1 → 要解析形参和唯一 return */ &paramCnt); /* 回传形参个数 */

	/* block() 结束后：形参个数和入口地址已准备好 */
	table[curFuncIdx].paramCnt = paramCnt; /* 写入符号表 */
	dx = savedDx;
}

/*
 * 编译程序主体
 *
 * lev:    当前分程序所在层
 * tx:     符号表当前尾指针
 * fsys:   当前模块后继符号集合
 */
void block(int *ptx, bool *fsys,
		   int isFunc,		 /* 1 = 函数体, 0 = main/普通块 */
		   int *retParamCnt) /* 仅 isFunc==1 时才用来回传形参个数 */
{
	int tx0 = *ptx; /* 记录本层符号表基准 */

	/* ---------- 1. 只有函数体才预留入口 jmp ---------- */
	int cx0 = -1; /* 非函数体保持 -1 */
	if (isFunc)
	{
		cx0 = cx;	 /* 记录 jmp 指令下标，稍后回填 */
		gen(jmp, 0); /* 先占个坑 */
	}

	/* ---------- 2. 解析形参 (仅 isFunc==1) ---------- */
	if (isFunc && retParamCnt)
	{
		*retParamCnt = 0;
		if (sym == lparen)
		{
			do
			{
				getsym(); /* 读标识符 */
				if (sym != ident)
					error(1);

				enter(param, ptx, &dx); /* adr = dx++ */
				(*retParamCnt)++;

				getsym(); /* 读下一个符号 */
			} while (sym == comma);

			if (sym != rparen)
				error(22);
			getsym(); /* 越过 ')' */
		}
	}

	/* ---------- 3. 进入 { stmt_list } ---------- */
	if (sym != lbrace)
		error(34);
	getsym(); /* 越过 '{' */

	int ini_pos = cx; /* 记录下这条 ini 指令所在的 code[] 下标 */
	gen(ini, 0);	  /* 暂时填 0，后续在第 6 步回填成正确的 dx */
	bool inside[symnum];
	addset(inside, statbegsys, fsys, symnum);
	inside[rbrace] = true;	  /* 块结束符 */
	inside[semicolon] = true; /* 语句间分号 */

	while (inset(sym, statbegsys))
	{
		statement(inside, ptx, &dx);
		if (sym == semicolon)
			getsym(); /* 可选分号 */
	}

	/* ---------- 4. 解析唯一 return (仅函数体) ---------- */
	if (isFunc)
	{
		if (sym != returnsym)
			error(50);
		getsym();

		bool exprFollow[symnum];
		addset(exprFollow, facbegsys, fsys, symnum);
		exprFollow[semicolon] = true; /* ; 跟在表达式后 */
		expression(exprFollow, ptx);
		gen(sto, 2);
		if (sym != semicolon)
			error(10);

		getsym();
	}

	/* ---------- 5. 读块结束 '}' ---------- */
	if (sym != rbrace)
		error(24);
	getsym();

	/* ---------- 6. 回填入口 & 生成退出指令 ---------- */
	if (isFunc)
	{
		code[cx0].a = ini_pos; /* jmp 跳到正文首指令 */
		code[ini_pos].a = dx;  /* +1 给返回值槽 */
		gen(opr, 18);		   /* return */
		table[curFuncIdx].size = dx - 3 + 1;
	}
	else
	{
		code[ini_pos].a = dx;
		gen(opr, 0); /* 程序或块结束 */
	}
}

/*
 * 在符号表中加入一项
 *
 * k:      标识符的种类为const，var或procedure
 * ptx:    符号表尾指针的指针，为了可以改变符号表尾指针的值
 * lev:    标识符所在的层次
 * pdx:    dx为当前应分配的变量的相对地址，分配后要增加1
 *
 */
int enter(enum object k, int *ptx, int *pdx)
{
	if (*ptx + 1 >= txmax)
	{
		printf("Symbol table overflow!\n");
		exit(1);
	}

	++(*ptx);
	strcpy(table[*ptx].name, id);
	table[*ptx].kind = k;
	table[*ptx].size = 0;
	table[*ptx].paramCnt = 0;
	table[*ptx].attr = 0;

	switch (k)
	{
	case variable:					/* let 声明 */
		table[*ptx].adr = (*pdx)++; /* 从 dx 开始递增 */
		break;

	case param:						/* 形参 */
		table[*ptx].adr = (*pdx)++; /* 与变量共用地址系 */
		table[*ptx].attr = 1;		/* bit0 = isParam  */
		break;

	case function: /* function / main */
		/* 入口地址稍后在 block() 中回填 */
		table[*ptx].adr = 0;
		break;
	}
	return *ptx;
}

/*
 * 查找标识符在符号表中的位置，从tx开始倒序查找标识符
 * 找到则返回在符号表中的位置，否则返回0
 *
 * id:    要查找的名字
 * tx:    当前符号表尾指针
 */
int position(char *id, int tx)
{
	int i;
	strcpy(table[0].name, id);
	i = tx;
	while (strcmp(table[i].name, id) != 0)
	{
		i--;
	}
	return i;
}

/*
 * 输出目标代码清单
 */
void listcode(int cx0)
{
	int i;
	if (listswitch)
	{
		printf("\n");
		for (i = cx0; i < cx; i++)
		{
			printf("%d %s %d\n", i, mnemonic[code[i].f], code[i].a);
		}
	}
}

/*
 * 输出所有目标代码
 */
void listall()
{
	int i;
	if (listswitch)
	{
		for (i = 0; i < cx; i++)
		{
			printf("%d %s %d\n", i, mnemonic[code[i].f], code[i].a);
			fprintf(fcode, "%d %s %d\n", i, mnemonic[code[i].f], code[i].a);
		}
	}
}

void call_handle(int pos)
{
	if (table[pos].kind != function)
		error(70); /* 70: 尝试调用非函数 */

	int argCnt = 0;

	getsym(); /* 跳过 '(' */
	if (sym != rparen)
	{ /* 非空实参表 */
		while (1)
		{
			bool nxtlev[symnum];
			addset(nxtlev, facbegsys, NULL, symnum);
			nxtlev[comma] = true;
			nxtlev[rparen] = true;

			expression(nxtlev, &tx); /* 实参表达式求值，结果在栈顶 */
			argCnt++;

			gen(opr, 17);

			if (sym == comma)
			{
				getsym(); /* 继续下一实参 */
				continue;
			}
			else if (sym == rparen)
			{
				break; /* 实参列表结束 */
			}
			else
			{
				error(22); /* 缺少 ')' */
				break;
			}
		}
	}

	if (sym != rparen)
		error(22); /* 确保 ')' */
	getsym();	   /* 跳过 ')' */

	/* 形参与实参个数核对 */
	if (argCnt != table[pos].paramCnt)
		error(60); /* 60: 参数个数不符 */

	/* 生成调用指令 */
	gen(cal, table[pos].adr);

	if (sym != semicolon)
		error(10); /* 缺少分号 */
	getsym();	   /* 跳过 ';' */
}

/*
 * 语句处理
 */
void statement(bool *fsys, int *ptx, int *pdx)
{
	bool nxtlev[symnum]; /* FOLLOW 集合 */

	/* ---------- let 声明 ---------- */
	if (sym == letsym)
	{
		getsym(); /* 读取 ident */
		if (sym != ident)
			error(1);

		enter(variable, ptx, pdx); /* 建符号，adr = dx++ */
		int varIdx = *ptx;		   /* 记录下标 */

		getsym(); /* 看是否有初始化 */

		if (sym == becomes) /* let x = expr; */
		{
			getsym();
			addset(nxtlev, facbegsys, fsys, symnum);
			expression(nxtlev, ptx);
			gen(sto, table[varIdx].adr);
		}

		if (sym != semicolon)
			error(10);
		getsym();
	}

	/* ---------- 赋值 / 函数调用 ---------- */
	else if (sym == ident)
	{
		int i = position(id, *ptx);
		getsym();
		if (sym == becomes)
		{ /* ident 后面直接跟 '=' */
			if (table[i].kind == function)
				error(61); /* 函数名不能出现在赋值左边 */
			getsym();	   /* 跳过 '=' */
			bool nxtlev[symnum];
			addset(nxtlev, facbegsys, fsys, symnum);
			nxtlev[semicolon] = true; /* ; 可跟在表达式后 */
			expression(nxtlev, ptx);
			gen(sto, table[i].adr); /* 把值写回变量 */
			if (sym != semicolon)
				error(10);
			getsym(); /* 跳过 ';' */
		}
		else if (sym == lparen)
		{ /* ---- 函数调用 ---- */
			call_handle(i);
		}
		else
			error(33);
	}

	/* ---------- if 语句 ---------- */
	else if (sym == ifsym)
	{
		getsym();
		if (sym != lparen)
			error(23); /* 23：缺少 '(' */
		getsym();	   /* 吃掉 '(' */

		/* --- 解析 condition --- */
		addset(nxtlev, statbegsys, fsys, symnum);
		nxtlev[rparen] = true;
		condition(nxtlev, ptx);

		/* --- 强制检测右括号 --- */
		if (sym != rparen)
			error(22); /* 22：缺少 ')' */
		getsym();	   /* 吃掉 ')' */

		int cx1 = cx;
		gen(jpc, 0); /* 条件假跳转 */

		if (sym != lbrace)
			error(34);
		getsym();
		bool bodyFollow[symnum];
		addset(bodyFollow, statbegsys, fsys, symnum);
		bodyFollow[rbrace] = true;	  /* 右花括号算作语句结束 */
		bodyFollow[semicolon] = true; /* 语句之后可选分号 */

		/* 循环消化块内所有语句 */
		while (inset(sym, statbegsys))
		{
			statement(bodyFollow, ptx, pdx);
			if (sym == semicolon)
				getsym(); /* 如果当前符号是分号，就先把它吞掉 */
		}
		if (sym != rbrace)
			error(24); /* 块没有以 '}' 结束 */
		getsym();	   /* 吃掉 '}' */

		if (sym == elsesym) /* 可选 else */
		{
			int cx2 = cx;
			gen(jmp, 0);	  /* if 真分支跳过 else */
			code[cx1].a = cx; /* 回填 jpc */

			getsym(); /* 读 else */
			if (sym != lbrace)
				error(34);
			getsym();
			bool bodyFollow[symnum];
			addset(bodyFollow, statbegsys, fsys, symnum);
			bodyFollow[rbrace] = true;	  /* 右花括号算作语句结束 */
			bodyFollow[semicolon] = true; /* 语句之后可选分号 */

			/* 循环消化块内所有语句 */
			while (inset(sym, statbegsys))
			{
				statement(bodyFollow, ptx, pdx);
				if (sym == semicolon)
					getsym(); /* 如果当前符号是分号，就先把它吞掉 */
			}
			if (sym != rbrace)
				error(24);	  /* 块没有以 '}' 结束 */
			getsym();		  /* 吃掉 '}' */
			code[cx2].a = cx; /* 回填 jmp */
		}
		else
			code[cx1].a = cx; /* 无 else，直接回填 */
	}

	/* ---------- while 语句 ---------- */
	else if (sym == whilesym)
	{
		int cx0 = cx; /* 循环开头 */
		getsym();
		if (sym != lparen)
			error(23); /* 23：缺少 '(' */
		getsym();	   /* 吃掉 '(' */

		/* --- 解析 condition --- */
		addset(nxtlev, statbegsys, fsys, symnum);
		nxtlev[rparen] = true;
		condition(nxtlev, ptx);

		/* --- 强制检测右括号 --- */
		if (sym != rparen)
			error(22); /* 22：缺少 ')' */
		getsym();	   /* 吃掉 ')' */

		int cx1 = cx;
		gen(jpc, 0);

		if (sym != lbrace)
			error(34);
		getsym();
		bool bodyFollow[symnum];
		addset(bodyFollow, statbegsys, fsys, symnum);
		bodyFollow[rbrace] = true;	  /* 右花括号算作语句结束 */
		bodyFollow[semicolon] = true; /* 语句之后可选分号 */

		/* 循环消化块内所有语句 */
		while (inset(sym, statbegsys))
		{
			statement(bodyFollow, ptx, pdx);
			if (sym == semicolon)
				getsym(); /* 如果当前符号是分号，就先把它吞掉 */
		}
		if (sym != rbrace)
			error(24); /* 块没有以 '}' 结束 */
		getsym();	   /* 吃掉 '}' */

		gen(jmp, cx0);
		code[cx1].a = cx; /* 回填假跳转 */
	}

	/* ---------- input 语句 ---------- */
	else if (sym == inputsym)
	{
		getsym();
		if (sym != lparen)
			error(26);
		getsym();
		while (1)
		{
			if (sym != ident)
				error(1); /* 缺少标识符 */
			int i = position(id, *ptx);
			if (i == -1)
				error(11); /* 未声明的标识符 */

			/* 先生成 OPR 16 指令：读入一个整数到栈顶 */
			gen(opr, 16);

			/* 然后把栈顶的值存到该变量的地址 */
			gen(sto, table[i].adr);

			/* 读下一个符号，看是逗号还是右括号 */
			getsym();

			if (sym == comma)
			{
				getsym(); /* 吃掉 ','，处理下一个变量 */
				continue;
			}
			break; /* 不是逗号，就退出循环 */
		}

		if (sym != rparen)
			error(22);
		getsym();

		if (sym != semicolon)
			error(10);
		getsym();
	}

	/* ---------- output 语句 ---------- */
	else if (sym == outputsym)
	{
		getsym();
		if (sym != lparen)
			error(34);
		getsym();

		while (1)
		{
			bool exprFollow[symnum];
			addset(exprFollow, facbegsys, fsys, symnum);
			/* 在表达式后允许看到 ',' 或 ')' */
			exprFollow[comma] = true;
			exprFollow[rparen] = true;

			expression(exprFollow, ptx);
			/* 每解析完一个表达式，就生成一次“打印栈顶”指令 */
			gen(opr, 14); /* 14: 输出栈顶整数 */

			if (sym == comma)
			{
				getsym(); /* 逗号分隔，继续下一个表达式 */
				continue;
			}
			break; /* 没有逗号，就退出循环，准备读右括号 */
		}

		if (sym != rparen)
			error(22);
		getsym();

		if (sym != semicolon)
			error(10);
		getsym();
	}

	/* ---------- return 语句（仅函数体允许） ---------- */
	else if (sym == returnsym)
	{
		getsym();
		addset(nxtlev, facbegsys, fsys, symnum);
		expression(nxtlev, ptx); /* 返回值压栈 */

		gen(sto, 3);  /* 写到 b-1 处 (约定的返回槽) */
		gen(opr, 18); /* 操作 0 0 = 返回 */

		if (sym != semicolon)
			error(10);
		getsym();
	}
	/* ---------- try-catch 语句 ---------- */
	else if (sym == trysym)
	{
		getsym(); /* 跳过 try */

		/* ===== 1. 生成 “pushC” 占位 ===== */
		int litIdx = cx;
		gen(lit, 0);
		gen(opr, 19); /* 19: pushC，把 catchAddr 压入运行期 catch-栈 */

		/* ===== 2. 解析 try { … } ===== */
		if (sym != lbrace)
			error(34); /* 缺少 '{' */
		getsym();	   /* 吞掉 '{' */

		bool tryFollow[symnum];
		addset(tryFollow, statbegsys, fsys, symnum);
		tryFollow[rbrace] = true;

		while (inset(sym, statbegsys))
		{
			statement(tryFollow, ptx, pdx);
			if (sym == semicolon)
				getsym();
		}
		if (sym != rbrace)
			error(24); /* 缺少 '}' */
		getsym();	   /* 吞掉 '}' */

		/* try 正常走完，需要跳过 catch */
		gen(opr, 20); /* ★ 正常走完也要 popC */
		int jmpIdx = cx;
		gen(jmp, 0); /* 稍后回填到 catch 之后 */

		/* ===== 3. 解析 catch { … } ===== */
		if (sym != catchsym)
			error(88); /* 缺少 catch */
		getsym();

		if (sym != lbrace)
			error(34);
		getsym();

		int catchStart = cx;		 /* catch 起始地址，回填到 lit */
		code[litIdx].a = catchStart; /* 把 lit 0 改成真正地址 */

		bool catchFollow[symnum];
		addset(catchFollow, statbegsys, fsys, symnum);
		catchFollow[rbrace] = true;

		while (inset(sym, statbegsys))
		{
			statement(catchFollow, ptx, pdx);
			if (sym == semicolon)
				getsym();
		}
		if (sym != rbrace)
			error(24);
		getsym();

		gen(opr, 20); /* 20: popC，catch 执行完弹栈 */

		/* ===== 4. 回填 “跳过 catch” ===== */
		code[jmpIdx].a = cx;
	}

	else
	{
		/* 空语句或非法开头：交给上层 test() 处理 */
	}
}

/*
 * 表达式处理
 */
void expression(bool *fsys, int *ptx)
{
	enum symbol addop; /* 用于保存正负号 */
	bool nxtlev[symnum];

	if (sym == plus || sym == minus) /* 表达式开头有正负号，此时当前表达式被看作一个正的或负的项 */
	{
		addop = sym; /* 保存开头的正负号 */
		getsym();
		memcpy(nxtlev, fsys, sizeof(bool) * symnum);
		nxtlev[plus] = true;
		nxtlev[minus] = true;
		term(nxtlev, ptx); /* 处理项 */
		if (addop == minus)
		{
			gen(opr, 1); /* 如果开头为负号生成取负指令 */
		}
	}
	else /* 此时表达式被看作项的加减 */
	{
		memcpy(nxtlev, fsys, sizeof(bool) * symnum);
		nxtlev[plus] = true;
		nxtlev[minus] = true;
		term(nxtlev, ptx); /* 处理项 */
	}
	while (sym == plus || sym == minus)
	{
		addop = sym;
		getsym();
		memcpy(nxtlev, fsys, sizeof(bool) * symnum);
		nxtlev[plus] = true;
		nxtlev[minus] = true;
		term(nxtlev, ptx); /* 处理项 */
		if (addop == plus)
		{
			gen(opr, 2); /* 生成加法指令 */
		}
		else
		{
			gen(opr, 3); /* 生成减法指令 */
		}
	}
}

/*
 * 项处理
 */
void term(bool *fsys, int *ptx)
{
	enum symbol mulop; /* 用于保存乘除法符号 */
	bool nxtlev[symnum];

	memcpy(nxtlev, fsys, sizeof(bool) * symnum);
	nxtlev[times] = true;
	nxtlev[slash] = true;
	factor(nxtlev, ptx); /* 处理因子 */
	while (sym == times || sym == slash)
	{
		mulop = sym;
		getsym();
		factor(nxtlev, ptx);
		if (mulop == times)
		{
			gen(opr, 4); /* 生成乘法指令 */
		}
		else
		{
			gen(opr, 5); /* 生成除法指令 */
		}
	}
}

/*
 * 因子处理
 */
/* factor() —— 解析因子，支持函数调用、变量、数字和括号表达式 */
/* factor() —— 解析因子，支持函数调用、变量、数字和括号表达式 */
void factor(bool *fsys, int *ptx)
{
	int i, argCnt;
	bool nxtlev[symnum];

	/* 检测因子的开始符号 */
	facbegsys[rparen] = true;
	test(facbegsys, fsys, 77);

	while (inset(sym, facbegsys)) /* 循环处理一个完整的因子 */
	{
		if (sym == ident)
		{
			/* 标识符：要么是函数调用，要么是普通变量或形参 */
			i = position(id, *ptx);
			if (i == -1)
				error(11); /* 未声明标识符 */
			getsym();

			/* --------- 如果紧跟 '('，视为函数调用 --------- */
			if (sym == lparen)
			{
				/* 1. 初始化 nxtlev = facbegsys ∪ fsys */
				memset(nxtlev, 0, sizeof(nxtlev));
				addset(nxtlev, facbegsys, fsys, symnum);
				nxtlev[comma] = true;  /* ← 一定要允许逗号 */
				nxtlev[rparen] = true; /* ← 一定要允许右括号 */

				/* 2. 解析实参列表，每个 expression 都把值压栈 */
				argCnt = 0;
				getsym();		   /* 已读到 '(', 现在取下一个符号 */
				if (sym != rparen) /* 允许空实参 */
				{
					do
					{
						/* 传入已初始化的 nxtlev，保证 expression() 能正确停到逗号或右括号 */
						expression(nxtlev, ptx);
						argCnt++;
						gen(opr, 17); /* 参数传递指令 */
						if (sym == comma)
							getsym();
					} while (sym != rparen);

					if (sym != rparen)
						error(22); /* 缺少右括号 ')' */
				}
				getsym(); /* 越过 ')' */

				/* 3. 检查实参个数是否与表中记录一致 */
				// if (argCnt != table[i].paramCnt)
				// error(60);

				/* 4. 生成函数调用指令，返回值留在栈顶 */
				gen(cal, table[i].adr);
			}
			/* --------- 否则视为普通变量或形参 --------- */
			else
			{
				if (table[i].kind == function)
					error(61); /* 函数名不能直接作为值 */
				gen(lod, table[i].adr);
			}
		}
		else if (sym == number)
		{
			/* 因子是数字常量 */
			if (num > amax)
			{
				error(31); /* 数字越界 */
				num = 0;
			}
			gen(lit, num);
			getsym();
		}
		else if (sym == lparen)
		{
			/* 因子是括号内表达式 "( ... )" */
			/* 1. 初始化 nxtlev = facbegsys ∪ fsys */
			memset(nxtlev, 0, sizeof(nxtlev));
			addset(nxtlev, facbegsys, fsys, symnum);

			getsym(); /* 吃掉 '(' */
			/* 2. 设置子表达式的 FOLLOW 集 = fsys ∪ { ')' } */
			memcpy(nxtlev, fsys, sizeof(bool) * symnum);
			nxtlev[rparen] = true;
			expression(nxtlev, ptx);
			if (sym == rparen)
				getsym();
			else
				error(22); /* 缺少右括号 ')' */
		}
		else
		{
			/* 不合法的因子开头，退出循环 */
			break;
		}

		/* 因子处理结束后，检查下一个符号是否合法 */
		memset(nxtlev, 0, sizeof(nxtlev));
		nxtlev[lparen] = true;
		fsys[comma] = true;
		test(fsys, nxtlev, 77);
	}
}

/*
 * 条件处理
 */
void condition(bool *fsys, int *ptx)
{
	enum symbol relop;
	bool nxtlev[symnum];

	/* 逻辑表达式处理 */
	memcpy(nxtlev, fsys, sizeof(bool) * symnum);
	nxtlev[eql] = true;
	nxtlev[neq] = true;
	nxtlev[lss] = true;
	nxtlev[leq] = true;
	nxtlev[gtr] = true;
	nxtlev[geq] = true;
	expression(nxtlev, ptx);
	if (sym != eql && sym != neq && sym != lss && sym != leq && sym != gtr && sym != geq)
	{
		error(20); /* 应该为关系运算符 */
	}
	else
	{
		relop = sym;
		getsym();
		expression(fsys, ptx);
		switch (relop)
		{
		case eql:
			gen(opr, 8);
			break;
		case neq:
			gen(opr, 9);
			break;
		case lss:
			gen(opr, 10);
			break;
		case geq:
			gen(opr, 11);
			break;
		case gtr:
			gen(opr, 12);
			break;
		case leq:
			gen(opr, 13);
			break;
		}
	}
}

/*
 * 解释程序
 */
void interpret()
{
	int p = 0;			  /* 指令指针 */
	int b = 1;			  /* 指令基址 */
	int t = 0;			  /* 栈顶指针 */
	int k = 3;			  // 参数位置
	struct instruction i; /* 存放当前指令 */
	int s[stacksize];	  /* 栈 */
	int catchStack[stacksize];
	int cTop = 0;

	printf("Start l25\n");
	fprintf(fresult, "Start l25\n");
	s[0] = 0; /* s[0]不用 */
	s[1] = 0; /* 主程序的三个联系单元均置为0 */
	s[2] = 0;
	s[3] = 0;
	do
	{
		i = code[p]; /* 读当前指令 */
		p = p + 1;
		switch (i.f)
		{
		case lit: /* 将常量a的值取到栈顶 */
			t = t + 1;
			s[t] = i.a;
			break;
		case opr: /* 数学、逻辑运算 */
			switch (i.a)
			{
			case 0: /* 函数调用结束后返回 */
				t = b - 1;
				p = s[t + 3];
				b = s[t + 2];
				break;
			case 1: /* 栈顶元素取反 */
				s[t] = -s[t];
				break;
			case 2: /* 次栈顶项加上栈顶项，退两个栈元素，相加值进栈 */
				t = t - 1;
				s[t] = s[t] + s[t + 1];
				break;
			case 3: /* 次栈顶项减去栈顶项 */
				t = t - 1;
				s[t] = s[t] - s[t + 1];
				break;
			case 4: /* 次栈顶项乘以栈顶项 */
				t = t - 1;
				s[t] = s[t] * s[t + 1];
				break;
			case 5: /* 次栈顶项除以栈顶项 */
			{		/* 次栈顶 ÷ 栈顶 */
				if (s[t] == 0)
				{
					printf("** Runtime Error: Division by zero at instruction %d\n", p - 1);
					fprintf(fresult, "** Runtime Error: Division by zero at instruction %d\n", p - 1);

					if (cTop > 0)
					{						  /* ① 最近的 catch 存在 */
						p = catchStack[cTop]; /* ② 跳转到 catchStart —— 不弹栈！ */
						t--;				  /* ③ 丢掉 try 内部残留表达式 */
					}
					else
					{ /* 没任何 catch 可以处理 */
						exit(1);
					}
				}
				else
				{ /* 正常除法 */
					t--;
					s[t] = s[t] / s[t + 1];
				}
				break;
			}
			case 6: /* 栈顶元素的奇偶判断 */
				s[t] = s[t] % 2;
				break;
			case 8: /* 次栈顶项与栈顶项是否相等 */
				t = t - 1;
				s[t] = (s[t] == s[t + 1]);
				break;
			case 9: /* 次栈顶项与栈顶项是否不等 */
				t = t - 1;
				s[t] = (s[t] != s[t + 1]);
				break;
			case 10: /* 次栈顶项是否小于栈顶项 */
				t = t - 1;
				s[t] = (s[t] < s[t + 1]);
				break;
			case 11: /* 次栈顶项是否大于等于栈顶项 */
				t = t - 1;
				s[t] = (s[t] >= s[t + 1]);
				break;
			case 12: /* 次栈顶项是否大于栈顶项 */
				t = t - 1;
				s[t] = (s[t] > s[t + 1]);
				break;
			case 13: /* 次栈顶项是否小于等于栈顶项 */
				t = t - 1;
				s[t] = (s[t] <= s[t + 1]);
				break;
			case 14: /* 栈顶值输出 */
				printf("%d ", s[t]);
				fprintf(fresult, "%d ", s[t]);
				t = t - 1;
				break;
			case 15: /* 输出换行符 */
				printf("\n");
				fprintf(fresult, "\n");
				break;
			case 16: /* 读入一个输入置于栈顶 */
				t = t + 1;
				printf("?");
				fprintf(fresult, "?");
				scanf("%d", &(s[t]));
				fprintf(fresult, "%d\n", s[t]);
				break;
			case 17: /* 参数传入 */
				s[t + k] = s[t];
				k++;
				t--;
				break;
			case 18:
			{
				int retVal = s[b + 2]; /* 从 s[b+2] 中取出 testfunc 真正写好的返回值 */
				int oldB = s[b + 0];   /* 从 s[b+0] 中取出上一层的基址 (SL) */
				int oldP = s[b + 1];   /* 从 s[b+1] 中取出上一层的返回地址 (RA) */

				t = b - 1;	   /* 恢复栈顶到 cal 之前的状态 */
				t = t + 1;	   /* 先把 t 再往上拨 1，改写为 8 */
				s[t] = retVal; /* 把 21 写到 s[8] */
				// 上面做法让 “返回值” → 直接留在 s[8] 上
				// 之后再恢复 b、p：
				b = oldB;
				p = oldP;
				break;
			}
			case 19:					   // pushC
				catchStack[++cTop] = s[t]; // t 保存的是 catchStart
				t--;
				break;

			case 20: // popC
				cTop--;
				break;
			}
			break;
		case lod: /* 取相对当前过程的数据基地址为a的内存的值到栈顶 */
			t = t + 1;
			s[t] = s[b + i.a];
			break;
		case sto: /* 栈顶的值存到相对当前过程的数据基地址为a的内存 */
			s[b + i.a] = s[t];
			t = t - 1;
			break;
		case cal:		  /* 调用子过程 */
			s[t + 1] = b; /* 将本过程基地址入栈，即建立动态链 */
			s[t + 2] = p; /* 将当前指令指针入栈，即保存返回地址 */
			s[t + 3] = 0; /* 留出一个格子给返回值（初始化为0） */
			b = t + 1;	  /* 更新基地址 */
			p = i.a;	  /* 跳转到函数入口 */
			k = 3;		  /* ← 初始化参数搬运偏移量为4 */
			break;
		case ini: /* 在数据栈中为被调用的过程开辟a个单元的数据区 */
			t = t + i.a;
			break;
		case jmp: /* 直接跳转 */
			p = i.a;
			break;
		case jpc: /* 条件跳转 */
			if (s[t] == 0)
				p = i.a;
			t = t - 1;
			break;
		}
		{
			printf("  [after %s %d]  stack (t=%d, b=%d):", mnemonic[i.f], i.a, t, b);
			fprintf(fresult, "  [after %s %d]  stack (t=%d, b=%d):", mnemonic[i.f], i.a, t, b);
			for (int i = 1; i <= t; i++)
			{
				printf(" %d", s[i]);
				fprintf(fresult, " %d", s[i]);
			}
			printf("\n");
			fprintf(fresult, "\n");
		} /*输出所有栈*/
	} while (p != 0);
	printf("\nEnd l25\n");
	fprintf(fresult, "\nEnd l25\n");
}