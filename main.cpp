#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <string>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <utility>
#include <algorithm>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <functional> 
#include <cctype>
#include <locale>

#define STEP_IN_BYTES 1
#define INSTR_SIZE 3

using namespace std;
typedef std::map<int,int> intmap;
typedef std::map<string,int> strmap;
typedef struct tagSecretShare {
	int currInstr;
	int Op1;
	int Op2;
	int Op3;
	int nextInstr;
} SecretShare;


//TASK - Scroll to end, process data definitions, then get back to the compiler to use variable declarations (

// -----------------------     Global variables ------------------------------------------

extern string compileIntoSubleq(char*);

char commandSeparator = ';';
char operandSeparator = ' ';
char dataStart = '.';
char labelSign = ':';

int loadStart=0;
int PC=0;
int nextAddress = 0;
bool skip_leading_zeroes = false;

string Nextcell = string("Next"),  
	   Subleq_Nextcell = string("?"),
	   Subleq_Instr_Prefix = string("");

strmap labelAddresses;
intmap labelValues;
intmap memoryRefs;
string numlabel_decl;

vector<SecretShare> secret_shares;

//  --------------------------  Utilities ------------------------------------------------------------

string tostring (int num)
{
	ostringstream st;
	st << num;
	return st.str();
}

bool is_number(string s) // Negative numbers to be counted!!
{
    std::string::const_iterator it = s.begin();
	char start_char = *it;
	if (start_char=='-') ++it; //skip - 
	
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

bool is_word(string s) // Negative numbers to be counted!!
{
    std::string::const_iterator it = s.begin();
	char start_char = *it;
	int hasChar=0;
	
    while (it != s.end() && std::isalnum(*it)) 
    {
		if (std::isalpha(*it)) hasChar=1;
		++it;
	}
    return !s.empty() && it == s.end() && hasChar;
}

int eval(string expr)
{
	if (is_number(expr))
		return atoi(expr.c_str());

	if (is_word(expr))
	{
		if (labelAddresses.find(expr)!=labelAddresses.end()) //found var
		{
			int addr = labelAddresses[expr];
			return labelValues[addr];
		}
		else 
			return -1; 
	}

	if (expr.at(0)=='-')
		return -eval(expr.substr(1));
	
	if (expr.at(0)=='(')
	{
		string content = expr.substr(1,expr.length()-1);
		return eval(content);
	}

    return 0;
}

string convert_to_binary_string(string arg)
{
	string str;
    int value = atoi(arg.c_str());
	//if (!value)
		//return string("0");

    bool found_first_one = false;
    int bits = STEP_IN_BYTES*8;

    for (int current_bit = bits - 1; current_bit >= 0; current_bit--)
    {
        if ((value & (1 << current_bit)) != 0)
        {
            if (!found_first_one)
                found_first_one = true;
            str += '1';
        }
        else
        {
            if (!skip_leading_zeroes || found_first_one)
                str += '0';
        }
    }
	
    return str;
	//return arg;
}

// trim from start
static inline std::string ltrim(std::string s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string rtrim(std::string s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string trim(std::string s) {
        return ltrim(rtrim(s));
}

string replaceAll(string subject, const std::string search, const std::string replace) 
{
    size_t pos = 0;
    while((pos = subject.find(search, pos)) != std::string::npos) 
	{
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;
}



string setIP(string subject, int nextAddr)
{
	string res,absolute;

	if (is_number(subject))  // one number instructio ns like "0" 
	{
		res=convert_to_binary_string(subject);
		absolute = res;
	}

	else 
	{
		res = replaceAll(subject, Subleq_Nextcell, Nextcell); // Next+...
	    if (res.find("+")!=std::string::npos) // Next+Num
		{
			// convert the after-+ constant into the binary format
			istringstream is(res);
			string label,offset;
			getline(is, label, '+'); //Next...
			getline(is, offset, '+'); //constant
			res= label + "+" + convert_to_binary_string(offset);
            int rightEnd = atoi(offset.c_str());
			absolute = convert_to_binary_string(tostring(nextAddr + rightEnd)); 
		}
		else //a label
		{
			if (labelAddresses.find(res)!=labelAddresses.end())
			  absolute = convert_to_binary_string(tostring(labelAddresses[res]));
			else
			  absolute=res;
		}
	}
	//return res;
	return absolute;
}

vector<string> splitIntoLines(string asmcode)
{
    stringstream stream(asmcode);
    vector<std::string> res;
    while (1)
	{
        string line;
        std::getline(stream,line);
        if (!stream.good())
            break;
		line.erase(std::remove(line.begin(), line.end(), '\t'), line.end());
        res.push_back(line);
    }
    return res;
}

void handleDataDecl(string decl)  ///determine actual addresses for 
{
	static bool nextDeclared=0;
	if (decl.find(Subleq_Nextcell)==std::string::npos) // no ? in the string
	{
		string declaration;
	    string declarations = trim(replaceAll(decl, ".",""));
        istringstream parser(declarations);
	   
	    while (getline(parser, declaration, operandSeparator))
	      {
		   string varname,initexpr;
		   istringstream dp(declaration);
		   getline(dp,varname,labelSign); // varName
		   
		   if (!is_number(varname))
		   {
		     getline(dp,initexpr,labelSign); // initializer   
			 labelAddresses[varname] = PC;
		   }

           labelValues[PC] = eval(initexpr);

		   PC = PC+STEP_IN_BYTES;
	     }
	}

	else 
	{
		if (!nextDeclared)
		   {
			 labelAddresses[Nextcell] = PC;
			 labelValues[PC] = PC + 1; 
			 nextDeclared = 1;
		   }
		PC = PC+STEP_IN_BYTES;
	}
}


string parseData(string decl)
{
	string result = "", dataName="";
	int dataAddress,dataValue;
	SecretShare ss;
	

	if (decl.find(Subleq_Nextcell)!=std::string::npos) // it IS .?		
	{
	    dataName = Nextcell;
		dataAddress = labelAddresses[dataName];
		dataValue = labelValues[dataAddress];

		result = result + 
		  convert_to_binary_string(tostring(dataAddress)) +
		  labelSign + 
		  convert_to_binary_string(tostring(dataValue)) + 
		  "(#"+dataName + ") ";

        ss.currInstr = PC; 
	    ss.nextInstr = PC + STEP_IN_BYTES;
		ss.Op1 = dataAddress;
		ss.Op2 = dataAddress;
		ss.Op3 = dataValue;
		secret_shares.push_back(ss);
        PC = PC + STEP_IN_BYTES;
	}
	else
	{
	  
	  string declaration;
	  string declarations = trim(replaceAll(decl, ".",""));
      istringstream parser(declarations);
	 
	  while (getline(parser, declaration, operandSeparator))
	   {
		 dataName = "";
		 istringstream dp(declaration);
		 getline(dp,dataName,labelSign); 
		 
		 if (is_number(dataName))  // no assignments after a constant
			 result = result + convert_to_binary_string(dataName) + " ";
		 else
		   {
			 dataAddress = labelAddresses[dataName];
			 dataValue = labelValues[dataAddress];
		     result = result + convert_to_binary_string(tostring(dataAddress)) + labelSign + convert_to_binary_string(tostring(dataValue));		 
             result = result + "(#"+dataName + ") ";

			 ss.currInstr = PC; 
	         ss.nextInstr = PC + STEP_IN_BYTES;
		     ss.Op1 = dataAddress;
		     ss.Op2 = dataAddress;
		     ss.Op3 = dataValue;
		     secret_shares.push_back(ss);
		   }
		 PC = PC + STEP_IN_BYTES;
	   }
	}

	return result;
}

// --------------------------------- End of utilities -----------------------------------------


string convertRest(vector<string> operands)
{
	string op1,op2,op3,result;
	SecretShare ss;
	ss.currInstr = PC;
	ss.nextInstr = PC + STEP_IN_BYTES*INSTR_SIZE;

    nextAddress = PC+STEP_IN_BYTES;
    op1 = setIP(operands.at(0),nextAddress); // first operand always exists
	ss.Op1 = (int)strtol(op1.c_str(), NULL, 2);

	if (operands.size()==1)      
	{    
		op2 = op1;                               // if only one operand op1 then op2:=op1; label:=next; Note that constants are not used in one-op instructions
		op3 = convert_to_binary_string(tostring(nextAddress));
		ss.Op2 = (int)strtol(op2.c_str(), NULL, 2);
		ss.Op3 = (int)strtol(op3.c_str(), NULL, 2);
		//result = Subleq_Instr_Prefix + op1;
	}
	else if (operands.size()==2)//two operands: label:=next
      {
        nextAddress = PC+2*STEP_IN_BYTES;
		op2=setIP(operands.at(1),nextAddress);
		op3 = convert_to_binary_string(tostring(nextAddress));
		ss.Op2 = (int)strtol(op2.c_str(), NULL, 2);
		ss.Op3 = (int)strtol(op3.c_str(), NULL, 2);
		//result = Subleq_Instr_Prefix + op1 + " " + op2;
	  }
	else 
	  {
		nextAddress = PC+2*STEP_IN_BYTES;
		op2=setIP(operands.at(1),nextAddress);
		nextAddress = PC+3*STEP_IN_BYTES;
		op3=setIP(operands.at(2),nextAddress);
		ss.Op2 = (int)strtol(op2.c_str(), NULL, 2);
		ss.Op3 = (int)strtol(op3.c_str(), NULL, 2);
		//result = Subleq_Instr_Prefix + op1 + " " + op2 + " " + op3; 
	  }
	result = Subleq_Instr_Prefix + op1 + " " + op2 + " " + op3; //subleq op1 op2 label
	secret_shares.push_back(ss);
	return result;
}

string convertSubleqCommand(string command)
{
	char lastChar = *command.rbegin();
	char firstChar = ltrim(command).at(0);
	vector<string> operands;
    string part,result;
	istringstream is(command);

	if (firstChar==dataStart)
		return parseData(command);  // This line . Z:-1 Z0:0 t1:0 t2:0 x:30 y:20  - advance PC within

	if (lastChar==labelSign)  //label
	{
		string labName = command.substr(0,command.length()-1);
		PC  = PC + STEP_IN_BYTES; // each label occupies STEP_IN_BYTES 
		return convert_to_binary_string(tostring(labelAddresses[labName])) + labelSign;
	}

    while (getline(is, part, operandSeparator))
	   operands.push_back(part);
	result = convertRest(operands);
	PC = PC + STEP_IN_BYTES*INSTR_SIZE;
	return result;
}


string convertAll(string asmcode)
{
	vector<string> lines = splitIntoLines(asmcode);
	vector<string> subleqCommands,convertedCommands,paddedCommands;
	string code;

	subleqCommands.clear();
	convertedCommands.clear();
	paddedCommands.clear();

    labelAddresses.clear();
	labelValues.clear();
	numlabel_decl =". ";

	for (int i = 0; i < lines.size(); i++)
	{
		string currentLine = lines.at(i);
		istringstream is(currentLine);
        string part;
        while (getline(is, part, commandSeparator))
			subleqCommands.push_back(part);
	}
    // Line 0 always redirects us to the main point. Correct it...
	subleqCommands.erase(subleqCommands.begin());
	subleqCommands.insert(subleqCommands.begin(), std::string("0 0 sqmain"));

	// First pass: resolving data and label declarations
	PC=0;
	for (int i = 0; i < subleqCommands.size(); i++)
	{		
		string trimmedCommand = trim(subleqCommands.at(i));
		string part;
		
		char lastChar = *trimmedCommand.rbegin();
        char firstChar = ltrim(trimmedCommand).at(0);

        if (firstChar==dataStart) // data declaration - starts with .
		{
			handleDataDecl(trimmedCommand); // PC is advanced within the function
		}

		else if (lastChar==labelSign) // label declaration ends with :
		{
			string labName = trimmedCommand.substr(0,trimmedCommand.length()-1);
			labelAddresses[labName] = PC;
            PC = PC + STEP_IN_BYTES;
		}
		else  //just an ordinary instruction, do not process, just advance PC so far
			PC = PC + STEP_IN_BYTES*INSTR_SIZE;
	}


	// Second pass: process instructions, Next + ... offsets
	PC=0;
    for (int i = 0; i < subleqCommands.size(); i++)
	{
		string trimmedCommand = trim(subleqCommands.at(i));
		string convertedCommand = convertSubleqCommand(trimmedCommand);
		convertedCommands.push_back(convertedCommand);

        if (PC % 30==0)
			code+=convert_to_binary_string(tostring(PC)) + ":\n";

        if (*convertedCommand.rbegin()!=labelSign)
			convertedCommand = '\t' + convertedCommand + commandSeparator;

		if (convertedCommand.length()>0)
		  code+=convertedCommand + '\n';
	}

	return code;
}

int main(int ac, char *av[])
{
   int i = 0;
   if (ac<2)
   {
	   cout << "At least input file is needed";
	   return 0;
   }
   
   string subleq = compileIntoSubleq(av[1]);
   string padded = convertAll(subleq);

   if (ac==3)
   {
	   std::ofstream of(av[2]);
	   of<<padded;
   }
   else
	   cout<<padded;

   system("pause");
   return 0;
}