#include "AnalisadorSintatico.h"

AnalisadorSintatico::AnalisadorSintatico(AnalisadorLexico *lex){
	this->lex = lex;
	fim = false;
	lex->LerToken();
}

//procedimento CasaToken [...]
//receber como parâmetro o token esperado pela gramática
//compará-lo com o token corrente
bool AnalisadorSintatico::CasaToken(string expectedToken){
	//Se estes forem iguais
	if(token.compare(expectedToken) == 0){
		#ifdef DEBUG
			static int ident = 0;
			static bool newline = true;

			if(token == "begin" || token == "main"){ident += 1;}
			if(token == "end"){ident -= 1;}

			if(newline){
				for(int i=0; i<ident; i++) printf("\t");
			}
			printf("[%s (%s)]", lexema.c_str(), token.c_str());

			if(token == ";" || token == "begin" || token == "main" || token == "end"){
				printf("\n");
				newline = true;
			}
			else{printf(" "); newline = false;}
		#endif

		//o próximo token do programa será lido (chamada ao analisador léxico)
		int res = lex->LerToken();
		if(res == 0) fim=true;
	}
	//caso contrário
	//deverá ser emitida uma mensagem com o erro encontrado e a linha do programa onde este ocorreu, encerrando o processo de compilação.
	else{
		if(!fim){
			if(token == ""){
				printf("%d:fim de arquivo nao esperado.\n", linha);
				exit(0);
			}
			else{
				printf("%d:Token não esperado [%s].\n", linha, token.c_str());
				exit(0);
			}
		}
		fim = true;
		return false;
	}
}

// S →  { Decl | Const | Nulo } 'main' {Comm} 'end'
bool AnalisadorSintatico::S(){
	while(token == "decl" || token == "decl_const" || token == ";"){
		if(token == "decl"){
			Decl();
		}
		else if(token == "decl_const"){
			Const();
		}
		else{
			Nulo();
		}
	}
	CasaToken("main");
	while(token == "id" || token == "while" || token == "if" || token == "readln" || token == "write" || token == "writeln" || token == ";"){
		Comm();
	}
	CasaToken("end");

	if(token != ""){
		printf("%d:Token não esperado [%s].\n", linha, token.c_str());
	}
}

// Comm → ( Att | Rep | T | Read | Write | Nulo )
bool AnalisadorSintatico::Comm(){
	if(token == "id"){
		Att();
	}
	else if(token == "while"){
		Rep();
	}
	else if(token == "if"){
		T();
	}
	else if(token == "readln"){
		Read();
	}
	else if(token == "write" || token == "writeln"){
		Write();
	}
	else{
		Nulo();
	}
}

// Decl → tipo id [= Exp] {, id [= Exp]};
bool AnalisadorSintatico::Decl(){
	string tipo = lexema;

	CasaToken("decl");

	//Checar se já foi declarado
	if(token == "id"){
		if(lex->t->FindSimbolo(lexema) == "NULL"){
			if(tipo == "string"){
				lex->t->AddSimbolo(lexema, "id", CLASSE_VAR, TIPO_STRING);
			}
			else if(tipo == "integer"){
				lex->t->AddSimbolo(lexema, "id", CLASSE_VAR, TIPO_INTEIRO);
			}
			else if(tipo == "boolean"){
				lex->t->AddSimbolo(lexema, "id", CLASSE_VAR, TIPO_LOGICO);
			}
			else if(tipo == "byte"){
				lex->t->AddSimbolo(lexema, "id", CLASSE_VAR, TIPO_BYTE);
			}
		}
		else{
			printf("%d:identificador ja declarado [%s].\n", linha, lexema.c_str());
			exit(0);
		}
	}

	CasaToken("id");

	if(token == "="){
		CasaToken("=");
		int *tipo = new int(-1);
		Exp(tipo);
	}

	while (token == ",") {
		CasaToken(",");

		//Checar se já foi declarado
		if(token == "id"){
			if(lex->t->FindSimbolo(lexema) == "NULL"){
				lex->t->AddSimbolo(lexema, "id");
			}
			else{
				printf("%d:identificador ja declarado [%s].\n", linha, lexema.c_str());
				exit(0);
			}
		}

		CasaToken("id");

		if(token == "="){
			CasaToken("=");
			int *tipo = new int(-1);
			Exp(tipo);
		}
	}

	CasaToken(";");
}

// Const → 'const' id = Exp;
bool AnalisadorSintatico::Const(){
	CasaToken("decl_const");

	string tok = token;
	string lexe = lexema;

	CasaToken("id");
	CasaToken("=");
	int *tipo = new int(-1);
	Exp(tipo);
	CasaToken(";");

	//Checar se já foi declarado
	if(tok == "id"){
		if(lex->t->FindSimbolo(lexe) == "NULL"){
			lex->t->AddSimbolo(lexe, "id", CLASSE_CONST, *tipo);
		}
		else{
			printf("%d:identificador ja declarado [%s].\n", linha, lexe.c_str());
			exit(0);
		}
	}
}

// Att → id = Exp;
bool AnalisadorSintatico::Att(){
	if(lex->t->FindSimbolo(lexema) == "NULL"){
		printf("%d:identificador nao declarado [%s].\n", linha, lexema.c_str());
		exit(0);
	}
	CasaToken("id");
	CasaToken("=");
	int *tipo = new int(-1);
	Exp(tipo);
	CasaToken(";");
}

// Write → ( write | writeln ) '(' Exp {, Exp} ')';
bool AnalisadorSintatico::Write(){
	if(token == "write"){
		CasaToken("write");
	}
	else{
		CasaToken("writeln");
	}

	CasaToken("(");
	int *tipo = new int(-1);
	Exp(tipo);

	while(token == ","){
		CasaToken(",");
		int *tipoB = new int(-1);
		Exp(tipoB);
	}

	CasaToken(")");
	CasaToken(";");
}

// Read → readln(id);
bool AnalisadorSintatico::Read(){
	CasaToken("readln");
	CasaToken("(");
	CasaToken("id");
	CasaToken(")");
	CasaToken(";");
}

// Rep → while ( Exp ) ( Comm | begin {Comm} end )
bool AnalisadorSintatico::Rep(){
	CasaToken("while");
	CasaToken("(");

	int *tipo = new int(-1);

	Exp(tipo);
	CasaToken(")");

	if(token != "begin"){
		Comm();
	}
	else{
		CasaToken("begin");
		while(token != "end"){
			Comm();
		}
		CasaToken("end");
	}
}

// T → if ( Exp ) then ( Comm | begin {Comm} end ) [ else ( Comm | begin {Comm} end ) ]
bool AnalisadorSintatico::T(){
	CasaToken("if");
	CasaToken("(");
	int *tipo = new int(-1);
	Exp(tipo);
	CasaToken(")");
	CasaToken("then");

	if(token == "begin"){
		CasaToken("begin");
		while(token != "end"){
			Comm();
		}
		CasaToken("end");
	}
	else{
		Comm();
	}

	if(token == "else"){
		CasaToken("else");

		if(token == "begin"){
			CasaToken("begin");
			while(token != "end"){
				Comm();
			}
			CasaToken("end");
		}
		else{
			Comm();
		}
	}
}

// Nulo → ;
bool AnalisadorSintatico::Nulo(){
	CasaToken(";");
}

// Exp → ExpA {comparadorArit ExpA}
bool AnalisadorSintatico::Exp(int *tipo){
	ExpA(tipo);

	while(token == "comp"){
		string lexe = lexema;

		CasaToken("comp");
		int *tipoB = new int(-1);
		ExpA(tipoB);

		if(*tipo == TIPO_STRING && *tipoB == TIPO_STRING && lexe != "=="){
			printf("%d:tipos incompatíveis.\n", linha);
			exit(0);
		}
		else if((*tipo == TIPO_INTEIRO || *tipo == TIPO_BYTE) && (*tipoB != TIPO_INTEIRO && *tipoB != TIPO_BYTE)){
			printf("%d:tipos incompatíveis.\n", linha);
			exit(0);
		}
		else if((*tipoB == TIPO_INTEIRO || *tipoB == TIPO_BYTE) && (*tipo != TIPO_INTEIRO && *tipo != TIPO_BYTE)){
			printf("%d:tipos incompatíveis.\n", linha);
			exit(0);
		}
		// TODO: Pensar em redundância nesses -else if-

		*tipo = TIPO_LOGICO;
	}
}

// ExpA → [(+|-)] ExpB { (+|-|or) ExpB }
bool AnalisadorSintatico::ExpA(int *tipo){
	if(token == "+") CasaToken("+");
	else if(token == "-") CasaToken("-");

	ExpB(tipo);

	while(token == "+" || token == "-" || token == "or"){
		if(token == "+") CasaToken("+");
		else if(token == "-") CasaToken("-");
		else if(token == "or") CasaToken("or");

		int *tipoB = new int(-1);
		ExpB(tipoB);
	}
}

// ExpB → ExpC {(*|/|and) ExpC}
bool AnalisadorSintatico::ExpB(int *tipo){
	ExpC(tipo);

	while(token == "*" || token=="/" || token=="and"){
		if(token == "*") CasaToken("*");
		else if(token == "/") CasaToken("/");
		else if(token == "and") CasaToken("and");

		int *tipoB = new int(-1);
		ExpC(tipoB);

		/*if(*tipo == TIPO_STRING || *tipoB == TIPO_STRING) {
			printf("%d:tipos incompatíveis.\n", linha);
			exit(0);
		}*/
	}
}

// ExpC → [not]ExpD
bool AnalisadorSintatico::ExpC(int *tipo){
	if(token == "not"){
		CasaToken("not");
	}
	ExpD(tipo);
}

// ExpD → '('Exp')' | id | const
bool AnalisadorSintatico::ExpD(int *tipo){
	if(token == "("){
		CasaToken("(");
		Exp(tipo);
		CasaToken(")");
	}
	else if(token == "id"){
		if(lex->t->FindSimbolo(lexema) == "NULL"){
			printf("%d:identificador nao declarado [%s].\n", linha, lexema.c_str());
			exit(0);
		}

		*tipo = lex->t->GetSimbolo(lexema)->tipo;
		CasaToken("id");
	}
	else{
		*tipo = tipo_constante;
		CasaToken("const");
	}
}
