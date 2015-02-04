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
#define DUMMY_VALUE -100

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
extern string get_in_binary(char*);
extern void interpret (vector<string>);

char commandSeparator = ';';
char operandSeparator = ' ';
char dataStart = '.';
char labelSign = ':';

int loadStart=0;
int PC=0;
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

string convert_to_binary_string(int value)
{
	string str = "";
    //int value = atoi(arg.c_str());

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
        res.push_back(trim(line));
    }
    return res;
}

vector<vector<int>> splitIntoChunks(int subVectorSize, vector<int> v)
{
    vector<vector<int>> v2;
    vector<int>::iterator start,end;
	start = end = v.begin();
    while(end != v.end())
     {
       int step = std::min(subVectorSize,std::distance(end, v.end()));              
       std::advance(end, step);
       vector<int> aux(end - start);
       std::copy(start, end, aux.begin());
       std::advance(start, step);
       v2.push_back(aux);  
    }
	return v2;
}


// --------------------------------- End of utilities -----------------------------------------


string convertSubleqCommand(string command) // at most 3 operands
{
	vector<int> components;
    string part,result;
	istringstream is(command);

    while (getline(is, part, operandSeparator))
		components.push_back(atoi(part.c_str()));

	vector<vector<int>> chunks = splitIntoChunks(INSTR_SIZE, components);
	for (int chunk=0; chunk<chunks.size(); chunk++)
	{
	   vector<int> operands = chunks.at(chunk);
	   int nextPC = PC + operands.size()*STEP_IN_BYTES;
	   if (operands.size()<INSTR_SIZE)
		for (int i=operands.size(); i<INSTR_SIZE; i++)
			operands.push_back(DUMMY_VALUE);

	   SecretShare ss;
	   ss.currInstr = PC; 
	   ss.nextInstr = nextPC;
	   ss.Op1 = operands.at(0);
	   ss.Op2 = operands.at(1);
	   ss.Op3 = operands.at(2);

	   secret_shares.push_back(ss);	
       PC  = nextPC;
	   result = result + convert_to_binary_string(ss.Op1) + operandSeparator;
	   if (ss.Op2!=DUMMY_VALUE) 
		   result = result + convert_to_binary_string(ss.Op2) + operandSeparator;
	   else result = result + operandSeparator;
	   if (ss.Op3!=DUMMY_VALUE) 
		   result = result + convert_to_binary_string(ss.Op3) + operandSeparator;
	   else result = result + operandSeparator;
	}
	result = result + "\n";
	return result;
}


string processAll(string asmcode)
{
	vector<string> commands = splitIntoLines(asmcode);
	vector<string> convertedCommands;
	string code = "";

	convertedCommands.clear();

	for (int i = 0; i < commands.size(); i++)
	{
		string currentCommand = commands.at(i);
		string convertedCommand = convertSubleqCommand(currentCommand);	
        code = code + convertedCommand;
	}
	return code;
}

int main(int ac, char *av[])
{
   int i = 0;
   std::ofstream of;
   if (ac<2)
   {
	   cout << "At least input and output files are needed";
	   return 0;
   }

   string bin_asq = get_in_binary(av[1]);
   string processed = processAll(bin_asq);
   of.open(av[2],std::ofstream::out | std::ofstream::trunc);
   of << processed;
   of.close();
   //system("pause");

   return 0;
}