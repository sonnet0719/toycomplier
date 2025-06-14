/*
 * PL/0 complier program implemented in C
 * PL/0编译器（完整版本）
 * The program has been tested on Visual Studio 2022
 *
 * 使用方法：
 * 运行后输入PL/0源程序文件名
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

#define norw 10		  /* 保留字个数 */
#define txmax 100	  /* 符号表容量 */
#define nmax 14		  /* 数字的最大位数 */
#define al 10		  /* 标识符的最大长度 */
#define maxerr 30	  /* 允许的最多错误数 */
#define amax 2048	  /* 地址上界*/
#define levmax 3	  /* 最大允许过程嵌套声明层数*/
#define cxmax 200	  /* 最多的虚拟机代码数 */
#define stacksize 500 /* 运行时数据栈元素最多为500个 */

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
};
#define symnum 31

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
	int l;		/* 引用层与声明层的层次差 */
	int a;		/* 根据f的不同而不同 */
};

bool listswitch;				/* 显示虚拟机代码与否 */
bool tableswitch;				/* 显示符号表与否 */
char ch;						/* 存放当前读取的字符，getch 使用 */
enum symbol sym;				/* 当前的符号 */
char id[al + 1];				/* 当前ident，多出的一个字节用于存放0 */
int num;						/* 当前number */
int cc, ll;						/* getch使用的计数器，cc表示当前字符(ch)的位置 */
int cx;							/* 虚拟机代码指针, 取值范围[0, cxmax-1]*/
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
	int level;		  /* 所处层，仅const不使用 */
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
void gen(enum fct x, int y, int z);
void test(bool *s1, bool *s2, int n);
int inset(int e, bool *s);
int addset(bool *sr, bool *s1, bool *s2, int n);
int subset(bool *sr, bool *s1, bool *s2, int n);
int mulset(bool *sr, bool *s1, bool *s2, int n);
void block(int lev, int *ptx, bool *fsys, int isFunc, int *retParamCnt);
void interpret();
void factor(bool *fsys, int *ptx, int lev);
void term(bool *fsys, int *ptx, int lev);
void condition(bool *fsys, int *ptx, int lev);
void expression(bool *fsys, int *ptx, int lev);
void statement(bool *fsys, int *ptx, int lev, int *pdx);
void listcode(int cx0);
void listall();
int position(char *idt, int tx);
void enter(enum object k, int *ptx, int lev, int *pdx);
int base(int l, int *s, int b);

/* 主程序开始 */
int main()
{
	bool nxtlev[symnum];

	printf("Input pl/0 file?   ");
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

	getsym();

	addset(nxtlev, declbegsys, statbegsys, symnum);
	nxtlev[period] = true;
	block(0, 0, nxtlev, 0 /*isFunc*/, NULL); /* 处理分程序 */

	if (sym != period)
	{
		error(9);
	}

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
		printf("\n%d errors in pl/0 program!\n", err);
		fprintf(foutput, "\n%d errors in pl/0 program!\n", err);
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
	strcpy(word[0], "else");
	wsym[0] = elsesym;
	strcpy(word[1], "function");
	wsym[1] = funcsym;
	strcpy(word[2], "if");
	wsym[2] = ifsym;
	strcpy(word[3], "input");
	wsym[3] = inputsym;
	strcpy(word[4], "let");
	wsym[4] = letsym;
	strcpy(word[5], "main");
	wsym[5] = mainsym;
	strcpy(word[6], "output");
	wsym[6] = outputsym;
	strcpy(word[7], "program");
	wsym[7] = progsym;
	strcpy(word[8], "return");
	wsym[8] = returnsym;
	strcpy(word[9], "while");
	wsym[9] = whilesym;

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
			printf("Program is incomplete!\n");
			exit(1);
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
							if (sym != period)
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
 * y: instruction.l;
 * z: instruction.a;
 */
void gen(enum fct x, int y, int z)
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
	code[cx].l = y;
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

/*
 * 编译程序主体
 *
 * lev:    当前分程序所在层
 * tx:     符号表当前尾指针
 * fsys:   当前模块后继符号集合
 */
void block(int lev, int *ptx, bool *fsys, int isFunc, int *retParamCnt)
{
	int dx = 3;		/* SL DL RA 固定占 3 */
	int tx0 = *ptx; /* 记录本层符号表基准 */
	int cx0 = cx;	/* 回填用的跳转入口 */

	gen(jmp, 0, 0); /* 预留一条跳转，稍后填入口地址 */

	/* ---------- 解析形参 ---------- */
	if (isFunc)
	{
		*retParamCnt = 0;

		if (sym == lparen)
		{
			do
			{
				getsym(); /* 读取 ident */
				if (sym != ident)
					error(1); /* 缺少标识符 */

				enter(param, ptx, lev + 1, &dx); /* 写符号表，adr = dx++ */
				(*retParamCnt)++;

				getsym(); /* 读下一个符号 */
			} while (sym == comma);

			if (sym != rparen)
				error(22); /* 缺少 ')' */
			getsym();	   /* 越过 ')' */
		}
	}

	/* ---------- 进入 { stmt_list } ---------- */
	if (sym != lbrace)
		error(23);
	getsym(); /* 越过 '{' */

	{
		bool nxtlev[symnum];
		addset(nxtlev, statbegsys, fsys, symnum);
		nxtlev[rbrace] = true; /* 语句块结束符也算 FOLLOW */

		while (inset(sym, statbegsys)) /* 至少一条语句 */
		{
			statement(nxtlev, ptx, lev + 1, &dx);
			if (sym == semicolon)
				getsym(); /* 可选分号 */
		}
	}

	/* ---------- 解析唯一的 return ---------- */
	if (isFunc)
	{
		if (sym != returnsym)
			error(50); /* 函数体缺少 return */
		getsym();	   /* 越过 return */

		expression(fsys, ptx, lev + 1); /* 计算返回值 */

		/* 把返回值放到 b-1 (约定：形参结束前的位置) */
		gen(sto, 0, 0);

		if (sym != semicolon)
			error(10);
		getsym();
	}

	if (sym != rbrace)
		error(24); /* 缺少 '}' */
	getsym();	   /* 越过 '}' */

	/* ---------- 回填入口地址 & 开栈 ---------- */
	code[cx0].a = cx; /* 填入口地址 */

	/* 局部区大小 = dx-3；若函数需预留 1 格返回值，再 +1 */
	gen(ini, 0, (dx - 3) + (isFunc ? 1 : 0));

	/* 函数/主程序结束指令 */
	gen(opr, 0, 0); /* return */

	/* ---------- 出层：恢复符号表 & dx ---------- */
	*ptx = tx0;
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
void enter(enum object k, int *ptx, int lev, int *pdx)
{
	if (*ptx + 1 >= txmax)
	{
		printf("Symbol table overflow!\n");
		exit(1);
	}

	++(*ptx);
	strcpy(table[*ptx].name, id);
	table[*ptx].kind = k;
	table[*ptx].level = lev;
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
			printf("%d %s %d %d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
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
			printf("%d %s %d %d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
			fprintf(fcode, "%d %s %d %d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
		}
	}
}

/*
 * 语句处理
 */
void statement(bool *fsys, int *ptx, int lev, int *pdx)
{
	bool nxtlev[symnum]; /* FOLLOW 集合 */

	/* ---------- let 声明 ---------- */
	if (sym == letsym)
	{
		getsym(); /* 读取 ident */
		if (sym != ident)
			error(1);

		enter(variable, ptx, lev, pdx); /* 建符号，adr = dx++ */
		int varIdx = *ptx;				/* 记录下标 */

		getsym(); /* 看是否有初始化 */

		if (sym == becomes) /* let x = expr; */
		{
			getsym();
			addset(nxtlev, facbegsys, fsys, symnum);
			expression(nxtlev, ptx, lev);
			gen(sto, lev - table[varIdx].level, table[varIdx].adr);
		}

		if (sym != semicolon)
			error(10);
		getsym();
	}

	/* ---------- 赋值 / 函数调用 ---------- */
	else if (sym == ident)
	{
		int i = position(id, *ptx);
		if (i == -1)
			error(11); /* 未声明标识符 */
		getsym();

		if (sym == lparen) /* ——函数调用因子（可能作为独立语句） */
		{
			/* 解析实参列表并生成 cal 指令 */
			int argCnt = 0;
			getsym();
			if (sym != rparen)
			{
				do
				{
					addset(nxtlev, facbegsys, fsys, symnum);
					expression(nxtlev, ptx, lev);
					argCnt++;
					if (sym == comma)
						getsym();
				} while (sym != rparen);

				if (sym != rparen)
					error(22);
			}
			getsym();

			if (argCnt != table[i].paramCnt)
				error(60); /* 实参与形参个数不符 */

			gen(cal, lev - table[i].level, table[i].adr);

			if (sym != semicolon)
				error(10);
			getsym();
		}
		else if (sym == becomes) /* ——赋值语句  ident = expr ; */
		{
			if (table[i].kind == function)
				error(61); /* 不能给函数名赋值 */

			getsym();
			addset(nxtlev, facbegsys, fsys, symnum);
			expression(nxtlev, ptx, lev);
			gen(sto, lev - table[i].level, table[i].adr);

			if (sym != semicolon)
				error(10);
			getsym();
		}
		else
			error(13); /* ident 后既不是 '(' 也不是 '=' */
	}

	/* ---------- if 语句 ---------- */
	else if (sym == ifsym)
	{
		getsym();
		addset(nxtlev, statbegsys, fsys, symnum);
		condition(nxtlev, ptx, lev);

		int cx1 = cx;
		gen(jpc, 0, 0); /* 条件假跳转 */

		if (sym != lbrace)
			error(23);
		getsym();
		statement(statbegsys, ptx, lev, pdx); /* if 块至少一条语句 */

		if (sym != rbrace)
			error(24);
		getsym();

		if (sym == elsesym) /* 可选 else */
		{
			int cx2 = cx;
			gen(jmp, 0, 0);	  /* if 真分支跳过 else */
			code[cx1].a = cx; /* 回填 jpc */

			getsym(); /* 读 else */
			if (sym != lbrace)
				error(23);
			getsym();
			statement(statbegsys, ptx, lev, pdx);
			if (sym != rbrace)
				error(24);
			getsym();
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
		addset(nxtlev, statbegsys, fsys, symnum);
		condition(nxtlev, ptx, lev);

		int cx1 = cx;
		gen(jpc, 0, 0);

		if (sym != lbrace)
			error(23);
		getsym();
		statement(statbegsys, ptx, lev, pdx);
		if (sym != rbrace)
			error(24);
		getsym();

		gen(jmp, 0, cx0);
		code[cx1].a = cx; /* 回填假跳转 */
	}

	/* ---------- input 语句 ---------- */
	else if (sym == inputsym)
	{
		getsym();
		if (sym != lparen)
			error(23);
		getsym();
		if (sym != ident)
			error(1);

		int i = position(id, *ptx);
		if (i == -1)
			error(11);
		getsym();
		if (sym != rparen)
			error(22);
		getsym();

		gen(opr, 0, 16); /* 16: 读整型，放栈顶 */
		gen(sto, lev - table[i].level, table[i].adr);

		if (sym != semicolon)
			error(10);
		getsym();
	}

	/* ---------- output 语句 ---------- */
	else if (sym == outputsym)
	{
		getsym();
		if (sym != lparen)
			error(23);
		getsym();

		addset(nxtlev, facbegsys, fsys, symnum);
		expression(nxtlev, ptx, lev);

		if (sym != rparen)
			error(22);
		getsym();
		gen(opr, 0, 14); /* 14: 输出栈顶整数 */

		if (sym != semicolon)
			error(10);
		getsym();
	}

	/* ---------- return 语句（仅函数体允许） ---------- */
	else if (sym == returnsym)
	{
		getsym();
		addset(nxtlev, facbegsys, fsys, symnum);
		expression(nxtlev, ptx, lev); /* 返回值压栈 */

		gen(sto, 0, 0); /* 写到 b-1 处 (约定的返回槽) */
		gen(opr, 0, 0); /* 操作 0 0 = 返回 */

		if (sym != semicolon)
			error(10);
		getsym();
	}

	else
	{
		/* 空语句或非法开头：交给上层 test() 处理 */
	}
}

/*
 * 表达式处理
 */
void expression(bool *fsys, int *ptx, int lev)
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
		term(nxtlev, ptx, lev); /* 处理项 */
		if (addop == minus)
		{
			gen(opr, 0, 1); /* 如果开头为负号生成取负指令 */
		}
	}
	else /* 此时表达式被看作项的加减 */
	{
		memcpy(nxtlev, fsys, sizeof(bool) * symnum);
		nxtlev[plus] = true;
		nxtlev[minus] = true;
		term(nxtlev, ptx, lev); /* 处理项 */
	}
	while (sym == plus || sym == minus)
	{
		addop = sym;
		getsym();
		memcpy(nxtlev, fsys, sizeof(bool) * symnum);
		nxtlev[plus] = true;
		nxtlev[minus] = true;
		term(nxtlev, ptx, lev); /* 处理项 */
		if (addop == plus)
		{
			gen(opr, 0, 2); /* 生成加法指令 */
		}
		else
		{
			gen(opr, 0, 3); /* 生成减法指令 */
		}
	}
}

/*
 * 项处理
 */
void term(bool *fsys, int *ptx, int lev)
{
	enum symbol mulop; /* 用于保存乘除法符号 */
	bool nxtlev[symnum];

	memcpy(nxtlev, fsys, sizeof(bool) * symnum);
	nxtlev[times] = true;
	nxtlev[slash] = true;
	factor(nxtlev, ptx, lev); /* 处理因子 */
	while (sym == times || sym == slash)
	{
		mulop = sym;
		getsym();
		factor(nxtlev, ptx, lev);
		if (mulop == times)
		{
			gen(opr, 0, 4); /* 生成乘法指令 */
		}
		else
		{
			gen(opr, 0, 5); /* 生成除法指令 */
		}
	}
}

/*
 * 因子处理
 */
void factor(bool *fsys, int *ptx, int lev)
{
	int i, argCnt;
	bool nxtlev[symnum];
	test(facbegsys, fsys, 24);	  /* 检测因子的开始符号 */
	while (inset(sym, facbegsys)) /* 循环处理因子 */
	{
		if (sym == ident)
		{
			i = position(id, *ptx);
			if (i == -1)
				error(11); /* 未声明标识符 */
			getsym();

			/* ---------- 函数调用作为因子 ---------- */
			if (sym == lparen)
			{
				/* 1. 解析实参列表，每个 expression 都把值压栈 */
				argCnt = 0;
				getsym(); /* 读 '(' 后第一个符号 */

				if (sym != rparen) /* 允许空实参表 */
				{
					do
					{
						addset(nxtlev, facbegsys, fsys, symnum);
						expression(nxtlev, ptx, lev);
						argCnt++;
						if (sym == comma)
							getsym();
					} while (sym != rparen);

					if (sym != rparen)
						error(22); /* 缺少 ')' */
				}
				getsym(); /* 越过 ')' */

				/* 2. 检查形参个数 */
				if (argCnt != table[i].paramCnt)
					error(60); /* 实参与形参个数不符 */

				/* 3. 生成调用指令，返回值留在栈顶 */
				gen(cal, lev - table[i].level, table[i].adr);
			}
			/* ---------- 普通变量 / 形参 ---------- */
			else
			{
				if (table[i].kind == function)
					error(61); /* 函数名不能直接用作值 */
				gen(lod, lev - table[i].level, table[i].adr);
			}
		}
		else
		{
			if (sym == number) /* 因子为数 */
			{
				if (num > amax)
				{
					error(31); /* 数越界 */
					num = 0;
				}
				gen(lit, 0, num);
				getsym();
			}
			else
			{
				if (sym == lparen) /* 因子为表达式 */
				{
					getsym();
					memcpy(nxtlev, fsys, sizeof(bool) * symnum);
					nxtlev[rparen] = true;
					expression(nxtlev, ptx, lev);
					if (sym == rparen)
					{
						getsym();
					}
					else
					{
						error(22); /* 缺少右括号 */
					}
				}
			}
		}
		memset(nxtlev, 0, sizeof(bool) * symnum);
		nxtlev[lparen] = true;
		test(fsys, nxtlev, 23); /* 一个因子处理完毕，遇到的单词应在fsys集合中 */
								/* 如果不是，报错并找到下一个因子的开始，使语法分析可以继续运行下去 */
	}
}

/*
 * 条件处理
 */
void condition(bool *fsys, int *ptx, int lev)
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
	expression(nxtlev, ptx, lev);
	if (sym != eql && sym != neq && sym != lss && sym != leq && sym != gtr && sym != geq)
	{
		error(20); /* 应该为关系运算符 */
	}
	else
	{
		relop = sym;
		getsym();
		expression(fsys, ptx, lev);
		switch (relop)
		{
		case eql:
			gen(opr, 0, 8);
			break;
		case neq:
			gen(opr, 0, 9);
			break;
		case lss:
			gen(opr, 0, 10);
			break;
		case geq:
			gen(opr, 0, 11);
			break;
		case gtr:
			gen(opr, 0, 12);
			break;
		case leq:
			gen(opr, 0, 13);
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
	struct instruction i; /* 存放当前指令 */
	int s[stacksize];	  /* 栈 */

	printf("Start pl0\n");
	fprintf(fresult, "Start pl0\n");
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
				t = t - 1;
				s[t] = s[t] / s[t + 1];
				break;
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
				printf("%d", s[t]);
				fprintf(fresult, "%d", s[t]);
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
			}
			break;
		case lod: /* 取相对当前过程的数据基地址为a的内存的值到栈顶 */
			t = t + 1;
			s[t] = s[base(i.l, s, b) + i.a];
			break;
		case sto: /* 栈顶的值存到相对当前过程的数据基地址为a的内存 */
			s[base(i.l, s, b) + i.a] = s[t];
			t = t - 1;
			break;
		case cal:						/* 调用子过程 */
			s[t + 1] = base(i.l, s, b); /* 将父过程基地址入栈，即建立静态链 */
			s[t + 2] = b;				/* 将本过程基地址入栈，即建立动态链 */
			s[t + 3] = p;				/* 将当前指令指针入栈，即保存返回地址 */
			b = t + 1;					/* 改变基地址指针值为新过程的基地址 */
			p = i.a;					/* 跳转 */
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
	} while (p != 0);
	printf("End pl0\n");
	fprintf(fresult, "End pl0\n");
}

/* 通过过程基址求上l层过程的基址 */
int base(int l, int *s, int b)
{
	int b1;
	b1 = b;
	while (l > 0)
	{
		b1 = s[b1];
		l--;
	}
	return b1;
}
