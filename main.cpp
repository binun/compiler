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

using namespace std;

//TASK - Scroll to end, process data definitions, then get back to the compiler to use variable declarations (

// -----------------------     Global variables ------------------------------------------

extern string compileIntoSubleq(char*);

char commandSeparator = ';';
char operandSeparator = ' ';
char dataStart = '.';
char labelSign = ':';

int STEP_IN_BYTES = 1;
int loadStart=0;
int instrOffset=0;
int datadeclOffset=0;
int nextAddress = 0;

bool skip_leading_zeroes = false;
bool codeFinished=0;

string Nextcell = string("Next"),  
	   Subleq_Nextcell = string("?"),
	   NextLabel = string("[PC+1]"),  // default jump label is the next instruction
	   Subleq_Instr_Prefix = string("subleq ");

std::map<unsigned long,string> numlabels;
std::map<string,int> labelAddresses;
std::map<string,int> labelValues;
string numlabel_decl;

//  --------------------------  Utilities ------------------------------------------------------------

bool is_number(string& s) // Negative numbers to be counted!!
{
    std::string::const_iterator it = s.begin();
	char start_char = *it;
	if (start_char=='-') ++it; //skip - 
	
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

bool is_word(string& s) // Negative numbers to be counted!!
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

int eval(string& expr)
{
	if (is_number(expr))
		return atoi(expr.c_str());
	if (is_word(expr))
	{
		if (labelValues.find(expr)!=labelValues.end()) //we have it in values
			return labelValues[expr];
		else // definition not found
			return -1; //dummy
	}

	if (expr.at(0)=='-')
		return eval(expr.substr(1));
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
}

// trim from start
static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

string replaceAll(string subject, const std::string& search, const std::string& replace) 
{
    size_t pos = 0;
    while((pos = subject.find(search, pos)) != std::string::npos) 
	{
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;
}



string setIP(string& subject)
{
	string res,absolute;

	if (is_number(subject))
	{
		res=convert_to_binary_string(subject);
		absolute = res;
	}

	else 
	{
		res = replaceAll(subject, Subleq_Nextcell, Nextcell); // Next+...
	    if (res.find("+")!=std::string::npos) // Next+...
		{
			// convert the after-+ constant into the binary format
			istringstream is(res);
			string label,offset;
			getline(is, label, '+'); //Next...
			getline(is, offset, '+'); //constant
			res= label + "+" + convert_to_binary_string(offset);
			absolute = convert_to_binary_string(std::to_string(nextAddress + atoi(offset.c_str()))); 
		}
		else //a label - numeric or symbolic ...
		{
			if (labelAddresses.find(res)!=labelAddresses.end())
			  absolute = convert_to_binary_string(std::to_string(labelAddresses[res]));
			else
			  absolute=res;
		}
	}
	//return res;
	return absolute;
}

vector<string> splitIntoLines(string &asmcode)
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

string handleNumber(string& numStr)
{
	int num = atoi(numStr.c_str());
	if (numlabels.find(num)==numlabels.end()) // not found		
	{
		numlabels[num] = convert_to_binary_string(numStr);
		numlabel_decl = "label" + numStr+":"+numlabels[num] + " ";
	}

	return "label" + numStr;
}

void collectDeclarations(string & decl)
{
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
		   labelAddresses[varname] = datadeclOffset;
		   if (!is_number(varname))
		   {
		     getline(dp,initexpr,labelSign); // initializer            
		   }
		   else //varname is a number - no subsequent parsing to get an initializer
		   {
			   initexpr = varname;
		   }

           labelValues[varname] = eval(initexpr);

		 datadeclOffset = datadeclOffset+STEP_IN_BYTES;
	   }
	}
}


string parseDataDecl(string & decl)
{
	string data = ". ";
	static bool nextDeclared=0;
	if (decl.find(Subleq_Nextcell)!=std::string::npos) // .?		
	   {
		   if (!nextDeclared)
		   {
			 data = data + Nextcell + labelSign + "0";
			 nextDeclared = 1;
		   }
		   else
			 data = std::string("");
	    }
	else
	{
	  string declaration;
	  string declarations = trim(replaceAll(decl, ".",""));
      istringstream parser(declarations);
	  
	  while (getline(parser, declaration, operandSeparator))
	   {
		 string varname,initexpr;
		 istringstream dp(declaration);
		 getline(dp,varname,labelSign); // varName
		 if (is_number(varname))        // no assignments after a constant
			 data = data + convert_to_binary_string(varname) + " ";
		 else
		   {
		     getline(dp,initexpr,labelSign); // initializer
			 data = data + varname+labelSign;
             data = data + convert_to_binary_string(std::to_string(eval(initexpr))) + " ";
		   }
		 datadeclOffset = datadeclOffset+STEP_IN_BYTES;
	   }
	}

	return data;
}

// --------------------------------- End of utilities -----------------------------------------


string convertRest(vector<string> & operands)
{
	string op1,op2,op3;
	string result;
	nextAddress = instrOffset+STEP_IN_BYTES;
	op1 = setIP(operands.at(0)); // first operand always exists
	if (operands.size()==1)      // if only one operand op1 then op2:=op1; label:=next; Note that constants are not used in one-op instructions
	{
		op2 = op1;
		op3 = convert_to_binary_string(std::to_string(nextAddress));
		//result = Subleq_Instr_Prefix + op1;
	}
	else if (operands.size()==2)//two operands: label:=next
      {
        nextAddress = instrOffset+2*STEP_IN_BYTES;
		op2=setIP(operands.at(1));
		op3 = convert_to_binary_string(std::to_string(nextAddress));
		//result = Subleq_Instr_Prefix + op1 + " " + op2;
	  }
	else 
	  {
		nextAddress = instrOffset+2*STEP_IN_BYTES;
		op2=setIP(operands.at(1));
		nextAddress = instrOffset+3*STEP_IN_BYTES;
		op3=setIP(operands.at(2));
		//result = Subleq_Instr_Prefix + op1 + " " + op2 + " " + op3; 
	  }
	instrOffset = instrOffset + operands.size()*STEP_IN_BYTES;
	result = Subleq_Instr_Prefix + op1 + " " + op2 + " " + op3; //subleq op1 op2 label

	return result;
}

string convertSubleqCommand(string & command)
{
	char lastChar = *command.rbegin();
	char firstChar = ltrim(command).at(0);
	vector<string> operands;
    string part,result;
	istringstream is(command);
    
	if (firstChar==dataStart)  // Check if this line is like . Z:-1 Z0:0 t1:0 t2:0 x:30 y:20  - Add binary declarations
       return parseDataDecl(command);

	if (lastChar==labelSign)  //label
		return command;

	if (is_number(command)) // numeric constant like 0
		return command;

    while (getline(is, part, operandSeparator))
	   operands.push_back(part);

	result = convertRest(operands);

	return "\t" + result + commandSeparator;
}



string convertAll(string asmcode)
{
	vector<string> lines = splitIntoLines(asmcode);
	vector<string> subleqCommands;
	vector<int> instrOffsets;
	string code;
	vector<string> convertedCommands;

	numlabels.clear();
    labelAddresses.clear();
	labelValues.clear();
	numlabel_decl =". ";
	instrOffset = 0;
	datadeclOffset=0;
	nextAddress = 0;

	for (int i = 0; i < lines.size(); i++)
	{
		string currentLine = lines.at(i);
		istringstream is(currentLine);
        string part;
        while (getline(is, part, commandSeparator))
			subleqCommands.push_back(part);
	}

	// Compute the code portion for setting data offset
	datadeclOffset=0;
	for (int i = 0; i < subleqCommands.size(); i++)
	{		
		string trimmedCommand = trim(subleqCommands.at(i));
		string part;
		
		char lastChar = *trimmedCommand.rbegin();
        char firstChar = ltrim(trimmedCommand).at(0);

        if (firstChar==dataStart) 
			continue;

		if (lastChar==labelSign)
		{
			string labName = trimmedCommand.substr(0,trimmedCommand.length()-1);
			labelAddresses[labName] = datadeclOffset;
		}

        istringstream is(trimmedCommand);
        vector<string> operands;
        while (getline(is, part, operandSeparator))
	        operands.push_back(part);
		datadeclOffset = datadeclOffset + operands.size()*STEP_IN_BYTES;
	}

	for (int i = 0; i < subleqCommands.size(); i++)
	{		
		string trimmedCommand = trim(subleqCommands.at(i));
		
		istringstream is(trimmedCommand);
        char firstChar = ltrim(trimmedCommand).at(0);
        if (firstChar==dataStart) 
			collectDeclarations(trimmedCommand);    
	}

    for (int i = 0; i < subleqCommands.size(); i++)
	{
		string currentCommand = subleqCommands.at(i);
		string trimmedCommand = trim(currentCommand);
		
		string convertedCommand = convertSubleqCommand(trimmedCommand); 
		instrOffsets.push_back(instrOffset);
		convertedCommands.push_back(convertedCommand);
		//if (convertedCommand.find(Subleq_Nextcell)!=std::string::npos) // .?
		//{
			//cout << convertedCommand;
	    //}
		
		if (convertedCommand.length()>0)
		 code+=convertedCommand + '\n';
	}

	code = code + numlabel_decl + '\n';
	//instrOffset = instrOffset+STEP_IN_BYTES; //one more command
	//datadeclOffset = instrOffset;
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
   string converted = convertAll(subleq);

   if (ac==3)
   {
	   std::ofstream of(av[2]);
	   of<<converted;
   }
   else
	   cout<<converted;

   system("pause");
   return 0;
}