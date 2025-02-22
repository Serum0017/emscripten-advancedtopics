#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <functional>
using namespace std;

#include <emscripten/val.h>

using emscripten::val;

// Use thread_local when you want to retrieve & cache a global JS variable once per thread.
thread_local const val document = val::global("document");

int main() {
    val canvas = document.call<val>("getElementById", "canvas");
    val ctx = canvas.call<val>("getContext", "2d");
    ctx.set("fillStyle", "green");
    ctx.call<void>("fillRect", 10, 10, 150, 100);
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
//     // // std::cout << "Hello C++!" << std::endl;

//     runCode(program);

//     return 0;
// }

void runCode(byte* program){
    // byte* readStorage = new byte[9];

    std::unordered_map<int, int> instructToLength;
    instructToLength[0] = 1;
    instructToLength[1] = 1;
    instructToLength[2] = 2;
    instructToLength[3] = 10;
    instructToLength[4] = 10;
    instructToLength[5] = 10;
    instructToLength[6] = 2;
    instructToLength[7] = 9;
    instructToLength[8] = 2;
    instructToLength[9] = 9;
    instructToLength[10] = 2;
    instructToLength[11] = 2;

    std::unordered_map<int, int> instructToMem;
    instructToMem[0] = 0;
    instructToMem[1] = 0;
    instructToMem[2] = 2;
    instructToMem[3] = 2;
    instructToMem[4] = 2;
    instructToMem[5] = 0;
    instructToMem[6] = 1;
    instructToMem[7] = 0;
    instructToMem[8] = 1;
    instructToMem[9] = 0;
    instructToMem[10] = 0;
    instructToMem[11] = 0;

    byte memory[4096];
    int pc = 0;// program counter
    int64_t registers[15];
    bool zeroFlag = false;
    bool overflowFlag = false;
    bool signedFlag = false;
    int statusCode = 1;

    while(true) {
        // FETCH
        byte instructionNum = program[pc];
        int instructionCode = ((int) instructionNum) >> 4;

        if(instructionCode==0){
            statusCode = 1;
            break;
        }
        if(instructionCode==1) continue;
        
        // first and second half of byte[1] of instruction
        int rA = (int) ((program[pc + 1]) >> 4);
        int rB = (int) (program[pc + 1] & ((byte) 0xF));

        int fnCode = (int) ((instructionNum) & ((byte) 0xF));
        int valP = pc+instructToLength[instructionCode];

        int valC = 0;
        if(instructToMem[instructionCode]!=0){
            for(int i = 0; i<8; i++){
                valC += (int) program[pc+instructToMem[instructionCode]+i];
                valC *= 8;
            }            
        }
        
        // DECODE
        
        int valA = registers[rA];
        int valB = registers[rB];
        int valE = 0;
        bool condition = false;

        int stackPtr = registers[4];

        //call, pop, push, ret
        //call
        if(instructionCode==8){
            valB = readDouble(memory,stackPtr);
        }
        if(instructionCode==9){
            valA = readDouble(memory,stackPtr);
            valB = readDouble(memory,stackPtr);
        }

        // EXECUTE
        switch(instructionCode){
            case 2:// cmov
                if(cmpSuccessful(zeroFlag, overflowFlag, signedFlag, fnCode)){
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
                zeroFlag = (valE == 0);
                signedFlag = (valE < 0);
                overflowFlag = ((valA<0) == (valB<0)) && ((valE < 0) != (valA < 0));
                break;
            case 7:// jXX
                condition = cmpSuccessful(zeroFlag, overflowFlag, signedFlag, fnCode);
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
                break;
            case 11:// pop
                break;
        }

        // MEMORY
        switch(instructionCode) {
            case 4:// rmmovq
                writeDouble(memory, valE, valA);
                // memory[valE] = valA;
                break;
            case 8:
                writeDouble(memory, valE, valP);
                // memory[valE] = valP;
            case 9:// ret
                valC = readDouble(memory, valA);
                break;
            case 10:// pushq
                // memory[valE]=valA;
                writeDouble(memory, valE, valA);
                break;
        }
        
        // WRITE BACK
        registers[rA] = valA;
        registers[rB] = valB;

        if(instructionCode == 8){
            registers[4] = valC;
        }
        if(instructionCode == 8){
            registers[4] = valE;
        }

        // PC
        if(condition){
            pc = valC;
            continue;
        }
        else{
            pc = valP;
        }
    }
}

int readDouble(byte* readFrom, int loc){
    int r = 0;
    for(int i = loc+8-1; i >= loc; i--){
        r *= 2;
        r += (int) readFrom[i];
    }
    return r;
}

void writeDouble(byte *writeTo, int loc, int val){
    for(int i = 0; i < 8; i++){
        writeTo[i + loc] = (byte) (((val) & (0xFF << (i * 8))) >> (i * 8));
    }
}

int opQ(int rA, int rB, int fnCode){
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
}// yes, le, l, e, ne, ge, g