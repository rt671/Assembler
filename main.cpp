#include <bits/stdc++.h>
using namespace std;
#define mem_no 15       //number of memory reference instructions
#define regref_no 12    //number of register reference instructions
#define reg_no 4        //number of processor registers

    bitset<14> memory[256];            //Memory (256*14)
    int CodeSeg=0, DataSeg=64, StackSeg=128, ExtraSeg=192;   //Segment Addresses (There are 4 segments)
    int LC=0, LC_DATA=0, line_no=1;     //Location Counter for instructions and data respectively, contain the OFFSET ADDRESS

    string line;
    char templine[50];
    char *token=NULL;

   ifstream input;

    struct label
   {
       bitset<8> address;   //8 bit address for 256 locations, highest two bits represent the segment address, the lower 6 bits represent offset
       char *label;
   };
   label symbolicAddressTable[60];  //SYMBOLIC ADDRESS TABLE (will store the labels and their location)
   int number=0;    //number of labels in program
   int number_prog=0; //number of labels in stored functions

   struct meminstruction
   {
       const char* instr;
       bitset<4> opcode;
   };
   meminstruction memoryref[mem_no];    //MEMORY REFERENCE INSTRUCTION TABLE

   struct reginstruction
   {
       const char* instr;
       bitset<13> opcode;
   };
   reginstruction regref[regref_no];    //REGISTER REFERENCE INSTRUCTION TABLE

   struct registers
   {
       const char* name;
       bitset<2> value;
   };
      registers reg[reg_no];    //  REGISTERS ACCESSIBLE BY THE PROGRAMMER

   struct storedproc
   {
       char* name;
   };
   int proc_no=0;
   storedproc procedure[10];    //LIST OF STORED PROCEDURES


void Error(int line_no, char* token)    //FUNCTION TO DETERMINE ERROR (if something is written beyond the instruction format)
{
    token=strtok(NULL, " ");
    if(token)
    {cout<<"LINE NUMBER "<<line_no<<" CONTAINS AN ERROR\n";
    exit(1);}
}

string breakline(string x, char delimiter)  //FUNCTION USED TO REMOVE THE COMMENTS
{
    string temp="";
    long i=0;
    while(x[i]!='\0')
    {
        if(x[i]!=delimiter){
            temp.push_back(x[i]);
        }
        else break;
        i++;
    }
    return temp;
}

int hexToInt(char *token)   //FUNCTION TO CONVERT HEXADECIMAL CHARACTER TO AN INTEGER
{
    int x=0;
    while ((*token)!='\0')
    {
        if (('0'<=(*token))&&((*token)<='9'))
            x= x*16 +(*token)-'0';
        else if (('A'<=(*token))&&((*token)<='F'))
            x= x*16 +(*token)-'A'+10;
        token++;
    }
    return x;
}

void mov(int k, char* token, int LC)    //FUNCTION TO SET THE BITS IN MEMORY WORD FOR MOV INSTRUCTION (for corresponding registers)
{
    for(int i=0;i<reg_no; i++)
    {
        if(!strcmp(token, reg[i].name))
            {
                memory[LC][2*k]=reg[i].value[1];
                memory[LC][2*k-1]=reg[i].value[0];
            }
    }
}

void FirstPass(int CS, int DS)
{
        bool procbegin=0;
        cout<<"PROGRAM:\n";
        LC=0, LC_DATA=0, line_no=1;
        bool data=0;
        while(getline(input, line))
        {
            if(line.length()==0) {procbegin=1; continue;}   //skip blank line
            cout<<line_no<<"\t"<<" "<<line<<endl;
            line=breakline(line, '/');                      //remove comments
            strcpy(templine, line.c_str());

            token=strtok(templine,",");          //token contains label if label is present, else it contains the whole line

              if(!strcmp(token,line.c_str()))
                {
                    token=strtok(templine, " ");
                    if(!strcmp(token, "ORG"))   //if not label, to check for ORG instruction as it sets LC
                    {
                        token=strtok(NULL, " ");
                        int x=hexToInt(token);
                        token=strtok(NULL, " ");
                        if(token && !strcmp(token, "DS"))   //to check, if it sets LC or LC_DATA
                             LC_DATA=x;                        //if DS present in instruction, sets LC_DATA
                        else
                            LC=x-1;
                    }
                    token=NULL;
                }
            if(token)   //true only for instructions containing label, now they will be filled in the table
            {
                int present_index=number;
                for(int i=number_prog; i<number;i++)    //checks if same label has already been listed in the table, if yes, it is overwritten
                {
                    if(!strcmp(token, symbolicAddressTable[i].label))
                    {present_index=i;    break;
                    }
                }
                char* temp= new char();
                strcpy(temp, token);
                symbolicAddressTable[present_index].label= temp;

                token=strtok(NULL, " ");
                if(!strcmp(token, "HEX")|| !strcmp(token, "DEC") || !strcmp(token, "BIN")) //if label represented data, segment address should be DS, not CS
                   {
                       symbolicAddressTable[present_index].address= bitset<8>(LC_DATA+DS);
                       data=1;
                   }
                else  symbolicAddressTable[present_index].address= bitset<8>(LC+CS);

                 if(procbegin)   {procedure[proc_no].name=temp;  proc_no++; procbegin=0;} //it is beginning of a stored procedure, store the label in stored procedure

                if(present_index==number) number++; //if they aren't equal, that means label was overwritten, so number remains same
            }
            if(!data)LC++;  else LC_DATA++;
            line_no++;
        }
        cout<<"\n----------FIRST PASS----------\n";
        cout<<"Symbolic address table\n";
        for(int i=number_prog; i<number;i++)
        {
            cout<<symbolicAddressTable[i].label<<" "<<symbolicAddressTable[i].address<<endl;
        }
        cout<<endl;
        cout<<"STORED PROCEDURES\n";
        for(int i=0; i<proc_no;i++)
        {
            cout<<procedure[i].name<<endl;
        }
}

void SecondPass(int CS, int DS){
        cout<<"\n--------SECOND PASS----------"<<endl;
        LC=0, LC_DATA=0, line_no=1;

        while(getline(input, line))
        {
            if(line=="") continue;  //skip blank line

            bool data=0;
            line=breakline(line, '/');  //remove comments
            strcpy(templine, line.c_str());

            token=strtok(templine,",");
            if(strcmp(token, line.c_str()))
                        token=strtok(NULL," ");
            else
            token=strtok(templine, " ");
            cout<<"Line no. "<<line_no<<": "<<token<<" Instruction"<<"; ";

            //CHECKING FOR PSEUDO-INSTRUCTION
            if(!strcmp(token, "ORG"))
                {
                    token=strtok(NULL, " ");
                    int x=hexToInt(token);
                    token=strtok(NULL, " ");
                    if(token && !strcmp(token, "DS"))   LC_DATA=x;
                    else   LC=x-1;
                    Error(line_no, token);
                }

            else if(!strcmp(token, "END"))
            {
                LC--;
                Error(line_no, token);
                break;
            }

            else if(!strcmp(token, "DEC") || !strcmp(token, "HEX") ||  !strcmp(token, "BIN"))   //if it is data
            {
                cout<<"Location in memory: "<<LC_DATA+DS<<endl;
                if(!strcmp(token, "DEC"))   {token=strtok(NULL, " ");   int x= atoi(token);  memory[LC_DATA+DS]=bitset<14>(x); }
                else if(!strcmp(token, "HEX")) {token=strtok(NULL, " "); int x=hexToInt(token);  memory[LC_DATA+DS]=bitset<14>(x); }
                else if(!strcmp(token, "BIN")) {token=strtok(NULL, " ");  string s(token);    memory[LC_DATA+DS]=bitset<14>(s);}
                data=1;
                cout<<"Value is "<<memory[LC_DATA + DS];
                Error(line_no, token);
            }

            else
                {
                    //CHECKING FOR MEMORY-REFERENCE INSTRUCTION
                    bool flag=0;
                    for(int i=0;i<mem_no;i++)
                    {
                        if(!strcmp(token, memoryref[i].instr))          //IF IT IS A MEMORY REFERENCE INSTRUCTION
                        {
                            flag=1;
                            cout<<"Location in memory: "<<LC+CS<<endl;
                            cout<<"Opcode is ";

                            for(int j=11;j>7;j--)          //bits 8,9,10,11 correspond to the opcode
                                {
                                    cout<<memoryref[i].opcode[j-8];
                                    memory[LC+CS][j]=memoryref[i].opcode[j-8];
                                }
                            cout<<"; ";

                            //SETTING ADDRESS OF THE OPERAND IN MEMORY
                            //if symbolic address is not present, then by default it will be 0000000
                             if(!strcmp(token, "MOV"))      //instruction format is different for MOV instruction
                            {
                                token=strtok(NULL, " ");
                                mov(2,token,LC+CS);
                                token=strtok(NULL, " ");
                                if(!strcmp(token, "I")){memory[LC+CS][12]=1; token=strtok(NULL, " ");}
                                mov(1,token,LC+CS);
                                token=strtok(NULL, " ");
                                if(token && !strcmp(token, "I")) memory[LC+CS][0]=1;
                                break;
                            }
                                //for instructions other than MOV
                            token=strtok(NULL, " ");
                            //FOR IMMEDIATE ADDRESSING MODE (store binary value in instruction; and set bit 13 which represents immediate addressing mode)
                            if(!strcmp(token,"DEC"))
                                {token=strtok(NULL, " ");  cout<<"Operand is ";
                                for(int i=6;i>=0;i--) {memory[LC+CS][i]=bitset<7>(atoi(token))[i]; cout<<memory[LC+CS][i];}
                                memory[LC+CS][13]=1;}

                            else if(!strcmp(token,"HEX"))
                                {token=strtok(NULL, " "); cout<<"Operand is ";
                                for(int i=6;i>=0;i--){ memory[LC+CS][i]=bitset<7>(hexToInt(token))[i]; cout<<memory[LC+CS][i];}
                                memory[LC+CS][13]=1;}

                            else if(!strcmp(token,"BIN"))
                                {token=strtok(NULL, " "); cout<<"Operand is ";
                                for(int i=6;i>=0;i--) {memory[LC+CS][i]=bitset<7>(string(token))[i]; cout<<memory[LC+CS][i];}
                                memory[LC+CS][13]=1;}

                            else{
                                for(int j=0;j<number;j++)
                                {
                                    if(!strcmp(token,symbolicAddressTable[j].label))//CHECK IN SYMBOLIC ADDRESS TABLE
                                    {
                                        cout<<"Address field: "<<symbolicAddressTable[j].address<<"; ";
                                        for(int k=7;k>=0;k--)    //settings bits 0-7 of memory
                                                memory[LC+CS][k]=symbolicAddressTable[j].address[k];
                                        break;
                                    }
                                }

                                //CHECKING FOR INDIRECT ADDRESSING MODE
                                token=strtok(NULL, " ");
                                    if(token && !strcmp(token, "I"))
                                        {memory[LC+CS][12]=1;  cout<<"Indirect Addressing Mode";}
                                Error(line_no, token);
                                break;
                                }//for else
                        }//for if
                    }//for for loop

                    if (flag==0){
                        //CHECKING FOR REGISTER REFERENCE INSTRUCTION
                        for(int i=0; i<regref_no; i++){
                             if(!strcmp(token, regref[i].instr))
                            {
                                 flag=1;
                                cout<<"Location in memory: "<<LC+CS<<endl;
                                cout<<"Opcode is ";
                                for(int j=12;j>=0;j--)
                                {
                                    cout<<regref[i].opcode[j];
                                    memory[LC+CS][j]=regref[i].opcode[j];
                                }
                                Error(line_no, token);
                            } }
                    if(flag==0) {cout<<"INVALID INSTRUCTION, ON LINE NUMBER "<<line_no<<endl; break;}
                    }
                }//for else
            if(!data)LC++; else LC_DATA++;  //if data, LC_DATA will be incremented, else LC
            line_no++;
            cout<<"\n\n";
        }
}
int main()
{
   //ASSIGNING NAMES TO REGISTERS
   reg[0].name="A"; //Accumulator
   reg[1].name="B"; //Temporary data register
   reg[2].name="C"; //Count register
   reg[3].name="BP";  //Base Pointer
   for(int i=0;i<reg_no; i++)
    reg[i].value=bitset<2>(i);

   //FILLING MEMORY REFERENCE INSTRUCTIONS TABLE
   memoryref[0].instr="LDAC";
   memoryref[1].instr="STAC";
   memoryref[2].instr="AND";
   memoryref[3].instr="OR";
   memoryref[4].instr="XOR";
   memoryref[5].instr="ADD";
   memoryref[6].instr="SUB";
   memoryref[7].instr="JMP";
   memoryref[8].instr="CALL";
   memoryref[9].instr="LOOP";
   memoryref[10].instr="INC";
   memoryref[11].instr="CMP";
   memoryref[12].instr="PUSH";
   memoryref[13].instr="POP";
   memoryref[14].instr="MOV";

   for(int i=0;i<mem_no; i++)
    memoryref[i].opcode=bitset<4>(i);

   //FILLING REGISTER REFERENCE INSTRUCTIONS TABLE
   regref[0].instr="ROTL";
   regref[1].instr="ROTR";
   regref[2].instr="CLAC";
   regref[3].instr="CMAC";
   regref[4].instr="RET";
   regref[5].instr="SZF";
   regref[6].instr="CLF";
   regref[7].instr="STF";
   regref[8].instr="ACIN";
   regref[9].instr="CIN";
   regref[10].instr="DAA";
   regref[11].instr="HLT";
   int k;
   regref[0].opcode=bitset<13>(3840);   //value of 0111100000000 in decimal is 3840
   for(int i=1;i<6; i++){
        k=pow(2,i-1);
    regref[i].opcode=bitset<13>(3840+k);
   }
   regref[6].opcode=bitset<13>(7936);   //value of 1111100000000 in decimal is 7936
   for(int i=7;i<regref_no; i++){
        k=pow(2,i-7);
    regref[i].opcode=bitset<13>(7936+k);
   }

    input.open("storedfun.txt");    //assembling the stored procedures
    cout.setstate(ios_base::failbit);
   if(input.is_open())
    {
        FirstPass(ExtraSeg , ExtraSeg+32);
        input.close();

        input.open("storedfun.txt");
        SecondPass(ExtraSeg , ExtraSeg+32);
        input.close();
        }
    else cout<<"Program cannot be opened";

    number_prog=number;
    cout.clear();

   input.open("program.txt");       //assembling the program input by user
   if(input.is_open())
    {
        FirstPass(CodeSeg, DataSeg);
        input.close();

        input.open("program.txt");
        SecondPass(CodeSeg, DataSeg);
        input.close();

        cout<<"\nMEMORY\n";
    for(int j=1;j<=4;j++){
            if(j==1)cout<<"CODE SEGMENT\n";
            else if(j==2)cout<<"DATA SEGMENT\n";
            else if(j==3)cout<<"STACK SEGMENT\n";
            else cout<<"EXTRA SEGMENT\n";
    for(int i=64*(j-1);i<64*j;i++)  cout<<i<<" "<<memory[i]<<endl;
    cout<<endl;}
    }
    else cout<<"Program cannot be opened";

    return 0;
}
