#include <iostream>
#include <fstream>
#include <iomanip>
#include <cctype>
#include <string>
#include <cstdlib>
#include <sstream>
using namespace std;

struct{
    string name;
    unsigned loc;
}SYMTAB[100];

bool  is_in_SYMTAB(string txt){
    for(int i=0;i<100;i++)if(txt==SYMTAB[i].name)return true;
    return false;
}

unsigned get_SYM_loc(string txt){
    for(int i=0;i<100;i++)if(txt==SYMTAB[i].name)return SYMTAB[i].loc;
    return 0;
}

void add_to_SYMTAB(string txt,int loc){
    static int use = 0;
    SYMTAB[use].name=txt;
    SYMTAB[use].loc = loc;
    use++;
}
typedef struct{
        unsigned char opcode;
        string symbol;
}Data;

Data  OPTAB[26]={
        {0x18,"ADD"},{0x40,"AND"},
        {0x28,"COMP"},{0x24,"DIV"},
        {0x3c,"J"},{0x30,"JEQ"},
        {0x34,"JGT"},{0x38,"JLT"},
        {0x48,"JSUB"},{0x00,"LDA"},
        {0x50,"LDCH"},{0x08,"LDL"},
        {0x04,"LDX"},{0x20,"MUL"},
        {0x44,"OR"},{0xd8,"RD"},
        {0x4c,"RSUB"},{0x0c,"STA"},
        {0x54,"STCH"},{0x14,"STL"},
        {0xe8,"STSW"},{0x10,"STX"},
        {0x1c,"SUB"},{0xe0,"TD"},
        {0x2c,"TIX"},{0xdc,"WD"}
};
bool is_in_OPTAB(string txt){
    for(int i=0;i<26;i++)if(txt==OPTAB[i].symbol)return true;
    return false;
}
unsigned get_objCode(string txt){
    for(int i=0;i<26;i++)if(txt==OPTAB[i].symbol)return OPTAB[i].opcode*0x010000;
    return 0;
}
string asciiToHex(char txt){
    string ans;
    ans+=(char)(txt/16);
    ans+=(char)(txt%16);
    return ans;
}
int htoi(string txt){
    int ans=0;
    for(unsigned int i=0;i<txt.length();i++){
        if(isdigit(txt[i]))ans=ans*16+txt[i]-'0';
        else ans=ans*16+txt[i]-'A'+10;
    }
    return ans;
}
fstream fin,listFile,OBJFile;
string TCardBuf;
unsigned TCardFrom,TCardLength;
unsigned int LocCTR;
string tmpString,label,opcode,data,programName;

void outputTCard(){
    string tmp;
    if(TCardLength!=0){
        OBJFile<<"T"<<setw(6)<<setfill('0')<<hex<<TCardFrom<<setw(2)<<setfill('0')<<hex<<TCardLength<<TCardBuf<<endl;
        TCardLength=0;
        TCardBuf="";
    }
};
void outT(unsigned address,string objCode){
    if(TCardLength+objCode.length()/2>30){
        outputTCard();
    }
    if(TCardLength==0){
        TCardFrom=address;
    }
    listFile<<"\t"<<objCode<<endl;
    TCardBuf.append(objCode);
    TCardLength+=objCode.length()/2;
}
void outT(unsigned address,unsigned objCode){
    stringstream ans;
    string tmp;
    ans<<hex<<setw(6)<<setfill('0')<<objCode;
    ans>>tmp;
    outT(address,tmp);
}

int main(){
    //Pass1
    fin.open("SRCFILE",ios::in);
    while(getline(fin,tmpString)){
        label="";
        opcode="";
        data="";
        for(unsigned int i=0;i<tmpString.length();i++){
            if(i<8&&tmpString[i]!=' ')label=label+tmpString[i];
            if(i>=9&&i<=14&&tmpString[i]!=' ')opcode+=tmpString[i];
            if(i>=17&&i<=34&&tmpString[i]!=' ')data+=tmpString[i];
        }
        if(opcode=="START")LocCTR=htoi(data.c_str());
        else {
            if(label!="")add_to_SYMTAB(label,LocCTR);

            if(is_in_OPTAB(opcode)==true)LocCTR+=3;
            else if(opcode=="WORD")LocCTR+=3;
            else if(opcode=="RESB")LocCTR+=atoi(data.c_str());
            else if(opcode=="RESW")LocCTR+= 3*atoi(data.c_str());
            else if(opcode=="BYTE"&&data[0]=='X')LocCTR+= (data.length()-3)/2;
            else if(opcode=="BYTE"&&data[0]=='C')LocCTR+= data.length()-3;
        }
    }
    fin.close();
    int ProgramEnd = LocCTR;

    //Pass2
    fin.open("SRCFILE",ios::in);
    listFile.open("LISTFILE",ios::out);
    OBJFile.open("OBJFILE",ios::out);
    LocCTR = 0;
    while(getline(fin,tmpString)){
        listFile<<hex<<LocCTR<<"\t"<<tmpString;
        label="";
        opcode="";
        data="";
        for(unsigned int i=0;i<tmpString.length();i++){
            if(i<8&&tmpString[i]!=' ')label=label+tmpString[i];
            if(i>=9&&i<=14&&tmpString[i]!=' ')opcode+=tmpString[i];
            if(i>=17&&i<=34&&tmpString[i]!=' ')data+=tmpString[i];
        }
        if(opcode=="START"){
            OBJFile<<"H"<<setw(6)<<setfill(' ')<<label<<setw(6)<<setfill('0')<<hex<<htoi(data)<<setw(6)<<setfill('0')<<(ProgramEnd-htoi(data) )<<endl;
            listFile<<endl;
            LocCTR=htoi(data);
            programName = data;
        }
        else if(is_in_OPTAB(opcode)==true){
            unsigned ans=get_objCode(opcode);
            if(data.substr(data.length()-2)==",X"){
                ans+=0x008000;
                data = data.substr(0,data.length()-2);
            }
            if(data!="")ans+= get_SYM_loc(data);
            outT(LocCTR,ans);
            LocCTR+=3;
        }
        else if(opcode=="WORD"){
            outT(LocCTR,atoi(data.c_str()));
            LocCTR+=3;
        }
        else if(opcode=="RESB"){
            outputTCard();
            listFile<<endl;
            LocCTR+=atoi(data.c_str());
        }
        else if(opcode=="RESW"){
            outputTCard();
            listFile<<endl;
            LocCTR+=3*atoi(data.c_str());
        }
        else if(opcode=="BYTE"&&data[0]=='X'){
            outT(LocCTR,data.substr(2,data.length()-3));
            LocCTR+=(data.length()-3)/2;
        }
        else if(opcode=="BYTE"&&data[0]=='C'){
            tmpString="";
            for(unsigned int i=2;i<data.length()-1;i++)tmpString+=asciiToHex(data[i]);
            outT(LocCTR,tmpString);
        }
        else if(opcode=="END"){
            if(data.length()!=0)OBJFile<<"E"<<hex<<setw(6)<<setfill('0')<<get_SYM_loc(data);
            else OBJFile<<"E"<<hex<<setw(6)<<setfill('0')<<get_SYM_loc(programName);
        }
    }
    OBJFile.close();
    listFile.close();
    return 0;
}
