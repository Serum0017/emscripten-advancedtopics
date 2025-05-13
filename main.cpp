#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <functional>
using namespace std;

#include <sstream>
#include "render.cpp"

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <signal.h>

using namespace emscripten;

std::unordered_map<int, int> instructToMem;
std::unordered_map<int, int> instructToLength;

int64_t readDouble(byte*, int64_t);
void writeDouble(byte*, int64_t, int64_t);
void scuffedWriteDouble(byte*, int64_t, int64_t);
int64_t opQ(int64_t, int64_t, int64_t);
bool cmpSuccessful(bool, bool, bool, int);

byte* program;
byte memoryData[128];
int pc = 0;// program counter
int64_t registers[15];
bool zeroFlag = false;
bool overflowFlag = false;
bool signedFlag = false;
int statusCode = 1;
int stepNum = 0;

std::vector<std::string> split(std::string& s, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        tokens.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(s);

    return tokens;
}

int main(){
    registers[4] = 128;//sizeof(memoryData) / 8 - 8; //might cause issues with push and pop
    instructToLength[0] = 1;
    instructToLength[1] = 1;
    instructToLength[2] = 2;
    instructToLength[3] = 10;
    instructToLength[4] = 10;
    instructToLength[5] = 10;
    instructToLength[6] = 2;
    instructToLength[7] = 9;
    instructToLength[8] = 9;
    instructToLength[9] = 1;
    instructToLength[10] = 2;
    instructToLength[11] = 2;

    instructToMem[0] = 0;
    instructToMem[1] = 0;
    instructToMem[2] = 0;
    instructToMem[3] = 2;
    instructToMem[4] = 2;
    instructToMem[5] = 2;
    instructToMem[6] = 0;
    instructToMem[7] = 1;
    instructToMem[8] = 1;
    instructToMem[9] = 0;
    instructToMem[10] = 0;
    instructToMem[11] = 0;

    initRender();
    return 0;
}

// int main() {
//     std::ifstream code;
//     code.open("file.s");
//     string line;

//     if(code.is_open()){}

//     std::getline(code, line);

//     int size = (line.size() / 8);
//     byte* program = new byte[size];
//     for(int i = 0; i<line.size(); i+=8){
//         for(int j = i; j < i+8; j++){
//             int pos = i / 8;
//             program[pos] = (byte) ((int) program[pos] + ((int) (line.at(j)-'0')));
//             program[pos] = (byte) ((int) program[pos] * (int) 2);
//         }
//     }
//     //

//     runCode(program);

//     return 0;
// }

void initCode(string line){

    std::vector<std::string> hexVals = split(line, " ");
    
    int len = hexVals.size();
    program = new byte[len];

    for(int i = 0; i < len; i++){
        unsigned int x;
        std::stringstream ss;
        ss << std::hex << hexVals[i];
        ss >> x;
        program[i] = (byte) x;
    }
}

std::unordered_map<string, int64_t> fetch(int64_t pc, int* statusCode, byte* program, std::unordered_map<int, int> instructToMem, std::unordered_map<int, int> instructToLength){
    // FETCH
    std::unordered_map<string, int64_t> returnDict;

    byte instructionNum = program[pc];
    int instructionCode = ((int) instructionNum) >> 4;

    if(instructionCode==0){
        *statusCode = 2;
        cout << "halted!\n";
        //raise(SIGSEGV);// CRASH AND BURN
        return returnDict;
    }
    
    // first and second half of byte[1] of instruction
    int rA = (int) ((program[pc + 1]) >> 4);
    int rB = (int) (program[pc + 1] & ((byte) 0xF));
    int fnCode = (int) ((instructionNum) & ((byte) 0xF));

    //Clear rA, rB for instructions which don't need it
    if(instructionCode == 0 || instructionCode == 1 || instructionCode == 7 || instructionCode == 8 || instructionCode == 9){
        rA = 15;
        rB = 15;
    }
    
    int valP = pc+instructToLength[instructionCode];

    int64_t valC = 0;

    if(instructToMem[instructionCode] != 0){
        for(int i = 7; i >= 0; i--){
            valC *= (1 << 8);
            valC += (int64_t) program[(int) pc+instructToMem[instructionCode]+i]; //Hopefully pc is never bigger than 2^32 (PRAY)
        }
    }

    // 0 0 0 0 0 0 12 34

    // 0 0 0 0 0 0 12

    // 9268

    // 0x2434

    // 4660

    // 11000100

    // 1234 in binary 1001000110100

    // 000000000000010

    //268435456
    //268435456 = 2 ** 28


    returnDict["rA"] = (int64_t) rA; //Casting to int64_t is OK here, we can just cast back to int later
    returnDict["rB"] = (int64_t) rB;
    returnDict["fnCode"] = (int64_t) fnCode;
    returnDict["valC"] = valC;
    returnDict["valP"] = valP;
    returnDict["instructionCode"] = (int64_t) instructionCode;

    return returnDict;
}

std::unordered_map<string, int64_t> decode(int64_t* registers, std::unordered_map<string, int64_t> packagedValues, byte* memoryData){
    // DECODE
    int rA = (int) packagedValues["rA"];
    int rB = (int) packagedValues["rB"];

    int64_t valA = registers[rA];
    int64_t valB = registers[rB];
    int64_t valE = 0;

    int stackPtr = registers[4];

    int instructionCode = packagedValues["instructionCode"];

    //call, pop, push, ret
    //call
    if(instructionCode==8){
        valB = stackPtr;
    }
    if(instructionCode==9){
        valA = stackPtr;
        valB = stackPtr;
    }
    if(instructionCode==10){
        valB = stackPtr;
    }
    if(instructionCode==11){
        valA = stackPtr;
        valB = stackPtr;
    }


    packagedValues["valA"] = valA;
    packagedValues["valB"] = valB;
    packagedValues["valE"] = valE;
    packagedValues["stackPtr"] = stackPtr;
    return packagedValues;

}

std::unordered_map<string, int64_t> execute(std::unordered_map<string, int64_t> packagedValues, bool* zeroFlag, bool* overflowFlag, bool* signedFlag){
    
    // EXECUTE
    
    int64_t valA = packagedValues["valA"];
    int64_t valB = packagedValues["valB"];
    int64_t valC = packagedValues["valC"];
    int64_t valE = packagedValues["valE"];
    int fnCode = (int) packagedValues["fnCode"];
    int instructionCode = (int) packagedValues["instructionCode"];
       
    bool condition = false;
    switch(instructionCode){
        case 2:// cmov
            if(cmpSuccessful(*zeroFlag, *overflowFlag, *signedFlag, fnCode)){
                valB = valA;
            }
            break;
        case 3:// irmovq
            valB = valC;
            break;
        case 4:// rmmovq
            valE = valB+valC;
            break;
        case 5:// mrmovq
            valE = valA+valC;
            break;
        case 6:// OPq
            valE = opQ(valA, valB, fnCode);
            *zeroFlag = (valE == 0);
            *signedFlag = (valE < 0);
            if(fnCode == 0){
                *overflowFlag = ((valA<0) == (valB<0)) && ((valE < 0) != (valA < 0));
            }
            else if(fnCode == 1){
                *overflowFlag = ((valA<0) == (valB>=0)) && ((valE < 0) != (valA < 0));
            }
            else{
                *overflowFlag = false;
            }
            
            break;
        case 7:// jXX
            condition = cmpSuccessful(*zeroFlag, *overflowFlag, *signedFlag, fnCode);
            break;
        case 8:// call
            condition = true;
            valE = valB - 8;
            break;
        case 9:// ret
            valE = valB+8;
            condition = true;
            break;
        case 10:// push
            valE = valB - 8;
            break;
        case 11:// pop
            valE = valB + 8;
            break;
    }

    packagedValues["valB"] = valB;
    packagedValues["valE"] = valE;
    packagedValues["condition"] = (int64_t) condition;
    return packagedValues;
}

std::unordered_map<string, int64_t> memory(std::unordered_map<string, int64_t> packagedValues, byte* memoryData){
    int instructionCode = (int) packagedValues["instructionCode"];
    
    int64_t valA = packagedValues["valA"];
    int64_t valP = packagedValues["valP"];
    int64_t valC = packagedValues["valC"];
    int64_t valE = packagedValues["valE"];
    int64_t valB = packagedValues["valB"];

    // MEMORY
    switch(instructionCode) {
        case 4:// rmmovq
            writeDouble(memoryData, valE, valA);
            // memory[valE] = valA;
            break;
        case 5:// mrmovq
            valB = readDouble(memoryData, valE);
            break;
        case 8:// call
            writeDouble(memoryData, valE, valP);
            break;
            // memory[valE] = valP;
        case 9:// ret
            valC = readDouble(memoryData, valA);
            break;
        case 10:// pushq
            // memory[valE]=valA;
            scuffedWriteDouble(memoryData, valE, valA);
            // for(int i = 0; i < 128; i++){
            //     cout << (int) memoryData[i] << "\n";
            // }
            break;
        case 11:// popq
            // memory[valE]=valA;
            valA = readDouble(memoryData, valA);
            break;
    }
    packagedValues["valA"] = valA;
    packagedValues["valC"] = valC;
    packagedValues["valB"] = valB;
    return packagedValues;
}
std::unordered_map<string, int64_t> writeBack(std::unordered_map<string, int64_t> packagedValues, int64_t* registers){
    //WRITE BACK

    int rA = (int) packagedValues["rA"];
    int rB = (int) packagedValues["rB"];
    int instructionCode = (int) packagedValues["rB"];
    

    if(rA!=15){
        registers[rA] = packagedValues["valA"];
    }
    if(rB!=15){
        // cout << packagedValues["valB"] <<" valB\n";
        // cout << packagedValues["rB"] <<" rB\n";
        registers[rB] = packagedValues["valB"];
    }
    
    if(packagedValues["instructionCode"] == 6){
        registers[rB] = packagedValues["valE"];
    }

    if(packagedValues["instructionCode"] == 8 || packagedValues["instructionCode"] == 9 || packagedValues["instructionCode"] == 10 || packagedValues["instructionCode"] == 11){
        registers[4] = (int64_t) packagedValues["valE"];
    }

    return packagedValues;
}

int64_t PC(int64_t pc, std::unordered_map<string, int64_t> packagedValues){

    // PC
    if((bool) packagedValues["condition"]){
        pc = packagedValues["valC"];
    }
    else{
        pc = packagedValues["valP"];
    }
    return pc;
}




// void runCode(byte* program){
//     // byte* readStorage = new byte[9];

//    std::unordered_map<int, int> instructToLength;
//     instructToLength[0] = 1;
//     instructToLength[1] = 1;
//     instructToLength[2] = 2;
//     instructToLength[3] = 10;
//     instructToLength[4] = 10;
//     instructToLength[5] = 10;
//     instructToLength[6] = 2;
//     instructToLength[7] = 9;
//     instructToLength[8] = 2;
//     instructToLength[9] = 9;
//     instructToLength[10] = 2;
//     instructToLength[11] = 2;

//     std::unordered_map<int, int> instructToMem;
//     instructToMem[0] = 0;
//     instructToMem[1] = 0;
//     instructToMem[2] = 2;
//     instructToMem[3] = 2;
//     instructToMem[4] = 2;
//     instructToMem[5] = 0;
//     instructToMem[6] = 1;
//     instructToMem[7] = 0;
//     instructToMem[8] = 1;
//     instructToMem[9] = 0;
//     instructToMem[10] = 0;
//     instructToMem[11] = 0;

//     byte memory[4096];
//     int pc = 0;// program counter
//     int64_t registers[15];
//     bool zeroFlag = false;
//     bool overflowFlag = false;
//     bool signedFlag = false;
//     int statusCode = 1;

//     while(true) {
//         // FETCH
//         byte instructionNum = program[pc];
//         int instructionCode = ((int) instructionNum) >> 4;

//         if(instructionCode==0){
//             statusCode = 1;
//             break;
//         }
//         if(instructionCode==1) continue;
        
//         // first and second half of byte[1] of instruction
//         int rA = (int) ((program[pc + 1]) >> 4);
//         int rB = (int) (program[pc + 1] & ((byte) 0xF));

//         int fnCode = (int) ((instructionNum) & ((byte) 0xF));
//          valP = pc+instructToLength[instructionCode];

//         int64_t valC = 0;
//         if(instructToMem[instructionCode]!=0){
//             for(int i = 0; i<8; i++){
//                 valC += (int64_t) program[pc+instructToMem[instructionCode]+i];
//                 valC *= 8;
//             }            
//         }
        
//         // DECODE
        
//         int64_t valA = registers[rA];
//         int64_t valB = registers[rB];
//         int64_t valE = 0;
//         bool condition = false;

//         int stackPtr = registers[4];

//         //call, pop, push, ret
//         //call
//         if(instructionCode==8){
//             valB = readDouble(memory,stackPtr);
//         }
//         if(instructionCode==9){
//             valA = readDouble(memory,stackPtr);
//             valB = readDouble(memory,stackPtr);
//         }

//         // EXECUTE
//         switch(instructionCode){
//             case 2:// cmov
//                 if(cmpSuccessful(zeroFlag, overflowFlag, signedFlag, fnCode)){
//                     valB = valA;
//                 }
//                 break;
//             case 3:// irmovq
//                 valB = valC;
//                 break;
//             case 4:// rmmovq
//                 valE = valB+valC;
//                 break;
//             case 5:// mrmovq
//                 valE = valA+valC;
//                 break;
//             case 6:// OPq
//                 valE = opQ(valA, valB, fnCode);
//                 zeroFlag = (valE == 0);
//                 signedFlag = (valE < 0);
//                 overflowFlag = ((valA<0) == (valB<0)) && ((valE < 0) != (valA < 0));
//                 break;
//             case 7:// jXX
//                 condition = cmpSuccessful(zeroFlag, overflowFlag, signedFlag, fnCode);
//                 break;
//             case 8:// call
//                 condition = true;
//                 valE = valB - 8;
//                 break;
//             case 9:// ret
//                 valE = valB+8;
//                 condition = true;
//                 break;
//             case 10:// push
//                 break;
//             case 11:// pop
//                 break;
//         }

//         // MEMORY
//         switch(instructionCode) {
//             case 4:// rmmovq
//                 writeDouble(memory, valE, valA);
//                 // memory[valE] = valA;
//                 break;
//             case 8:
//                 writeDouble(memory, valE, valP);
//                 // memory[valE] = valP;
//             case 9:// ret
//                 valC = readDouble(memory, valA);
//                 break;
//             case 10:// pushq
//                 // memory[valE]=valA;
//                 writeDouble(memory, valE, valA);
//                 break;
//         }
        
//         // WRITE BACK
//         registers[rA] = valA;
//         registers[rB] = valB;

//         if(instructionCode == 8){
//             registers[4] = valC;
//         }
//         if(instructionCode == 8){
//             registers[4] = valE;
//         }

//         // PC
//         if(condition){
//             pc = valC;
//             continue;
//         }
//         else{
//             pc = valP;
//         }
//     }
// }

void scuffedWriteDouble(byte *writeTo, int64_t loc, int64_t val){
    for(int i = 0; i < 8; i++){
        writeTo[i + loc] = (byte) (((unsigned) ((val) & (0xFF << (i * 8)))) >> (i * 8));
    }
}

void writeDouble(byte *writeTo, int64_t loc, int64_t val){
    for(int i = 0; i < 8; i++){
        writeTo[i + loc] = (byte) ((((val) & (0xFF << (i * 8)))) >> (i * 8));
    }
}

int64_t opQ(int64_t rA, int64_t rB, int64_t fnCode){
    switch (fnCode){
        case 0:// addq
            return rA+rB;
        case 1:// subq
            return rB-rA;
        case 2:// andq
            return rA & rB;
        case 3:// xorq
            return rA ^ rB;
    }
    return 0;
}

//Level step, byte* memoryData, int* pc, int64_t* registers, bool* zeroFlag, bool* overflowFlag, bool* signedFlag, int* statusCode, std::unordered_map<int, int> instructToMem, std::unordered_map<int, int> instructToLength, byte* readStorage
std::unordered_map<string, int64_t> packagedValues;
void step(){
    if(statusCode == 2){
        return;
    }
    switch(stepNum){
        case 0:
        //int64_t pc, int* statusCode, byte* program, std::unordered_map<int, int> instructToMem, std::unordered_map<int, int> instructToLength)
            packagedValues = fetch(pc, &statusCode, program, instructToMem, instructToLength);
            break;
        case 1:
        //decode(int64_t* registers, std::unordered_map<string, int64_t> packagedValues, byte* memoryData){
            packagedValues = decode(registers, packagedValues, (byte *) memoryData);
            break;
        case 2:
            packagedValues = execute(packagedValues, &zeroFlag, &overflowFlag, &signedFlag);
            break;
        case 3:
            packagedValues = memory(packagedValues, (byte *) memoryData);
            break;
        case 4:
            packagedValues = writeBack(packagedValues, registers);
            break;
        case 5:
            pc = PC(pc, packagedValues);
            break;
    }

    render(packagedValues, registers, stepNum, memoryData, pc, zeroFlag, overflowFlag, signedFlag, statusCode);

    stepNum = (stepNum + 1) % 6;
    
}

EMSCRIPTEN_BINDINGS(my_module) {
    // emscripten::function("lerp", &lerp);
    // emscripten::function("render", render);
    emscripten::function("step", &step);
    emscripten::function("initCode", &initCode);
}

bool cmpSuccessful(bool zeroFlag, bool overflowFlag, bool signedFlag, int fnCode){
    switch(fnCode){
        case 0:
            return true;
        case 1:
            return zeroFlag || (signedFlag ^ overflowFlag);
        case 2:
            return signedFlag ^ overflowFlag;
        case 3:
            return zeroFlag;
        case 4:
            return !zeroFlag;
        case 5:
            return !(signedFlag^overflowFlag);
        case 6:
            return !(signedFlag ^ overflowFlag) & !zeroFlag;
        
    }
    return false;
}// yes, le, l, e, ne, ge, g