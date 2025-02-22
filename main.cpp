#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <functional>
using namespace std;

#include <emscripten/bind.h>
#include <emscripten/val.h>

const int canvasW = 1600;
const int canvasH = 900;

using namespace emscripten;

int main() {
    // const auto document = emscripten::val::global("document");
    // const auto canvas = document.call<emscripten::val, std::string>("querySelector", "canvas");

    // auto ctx = canvas.call<emscripten::val, std::string>("getContext", "2d");
  
    // const auto width = canvas["width"].as<int>();
    // const auto height = canvas["height"].as<int>();
  
    // ctx.call<void>("clearRect", 0, 0, width, height);
  
    // // rect
    // ctx.set("fillStyle", "white");
    // ctx.call<void>("fillRect", 0, 0, width, height);
  
    // // line
    // ctx.set("strokeStyle", "black");
    // ctx.call<void>("moveTo", 0, 0);
    // ctx.call<void>("lineTo", width, height);
    // ctx.call<void>("stroke");
  
    // // text
    // ctx.set("fillStyle", "black");
    // ctx.set("font", "bold 48px serif");
    // ctx.call<void, std::string>("fillText", "Hello World!", width / 2, height / 2);
  
    return 0;
}

float rotation = 0;

void rotateAt(emscripten::val ctx, int x, int y, float angle){
    ctx.call<void>("translate", x, y);
    ctx.call<void>("rotate", angle);
    ctx.call<void>("translate", -x, -y);
}

const int registerW = 150;
const int registerH = canvasH / 15;
const int registerPadding = 5;
void renderRegister(emscripten::val ctx, int x, int y, long val, string registerName, bool isOdd){
    rotateAt(ctx, x + registerW/2, y + registerH/2, isOdd ? rotation : -rotation);

    ctx.set("strokeStyle", "blue");
    ctx.set("lineWidth", 2);
    ctx.call<void>("strokeRect", x + registerPadding, y + registerPadding, registerW - registerPadding*2, registerH - registerPadding*2);

    // string str = long2str()
    ctx.set("fillStyle", "black");
    ctx.set("font", "bold 48px serif");
    ctx.set("textAlign", "center");
    ctx.set("textBaseline", "middle");
    ctx.call<void>("fillText", registerName, x + registerW/2, y + registerH/2);
    
    // ctx.call<void>("beginPath");
    // ctx.set("strokeStyle", "red");
    // ctx.call<void>("moveTo", x, y);
    // ctx.call<void>("lineTo", x + registerW, y + registerH);
    // ctx.call<void>("stroke");
    // ctx.call<void>("closePath");

    rotateAt(ctx, x + registerW/2, y + registerH/2, isOdd ? -rotation : rotation);
}

std::string long2str(long w, size_t hex_len = sizeof(long)<<1) {
    static const char* digits = "0123456789ABCDEF";
    std::string rc(hex_len,'0');
    for (size_t i=0, j=(hex_len-1)*4 ; i<hex_len; ++i,j-=4)
        rc[i] = digits[(w>>j) & 0x0f];
    return rc;
}

void renderRegisters(emscripten::val ctx){
    const string registerNames[] = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r12", "r13"};
    int x = canvasW/2 - registerW/2;
    int y = 0;
    for(int i = 0; i < 15; i++){
        renderRegister(ctx, x, y, 0/*TODO val*/, registerNames[i], i % 2 == 1);
        y += registerH;
    }
}

void renderConditionCode(int x, int y, long val, string conditionCodeName){
    
}

void renderPC(int x, int y, long val){

}

void renderStatus(int x, int y, int status){
    
}

void renderStackTop(int x, int y, long val){
    
}

void renderStage(emscripten::val ctx, string stage){
    // text
    ctx.set("fillStyle", "black");
    ctx.set("font", "bold 48px serif");
    ctx.call<void>("fillText", stage, 20, 20);
}

void render(float dt) {// deltaTime, use for animations
    const auto document = emscripten::val::global("document");
    const auto canvas = document.call<emscripten::val, std::string>("querySelector", "canvas");

    auto ctx = canvas.call<emscripten::val, std::string>("getContext", "2d");

    // render background
    ctx.set("fillStyle", "white");
    ctx.call<void>("fillRect", 0, 0, canvasW, canvasH);

    rotation += dt;

    renderRegisters(ctx);
}

// float lerp(float a, float b, float t) {
//     return (1 - t) * a + t * b;
// }

EMSCRIPTEN_BINDINGS(my_module) {
    // emscripten::function("lerp", &lerp);
    emscripten::function("render", &render);
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

// void runCode(byte* program){
//     // byte* readStorage = new byte[9];

//     std::unordered_map<int, int> instructToLength;
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

//         long valC = 0;
//         if(instructToMem[instructionCode]!=0){
//             for(int i = 0; i<8; i++){
//                 valC += (long) program[pc+instructToMem[instructionCode]+i];
//                 valC *= 8;
//             }            
//         }
        
//         // DECODE
        
//         long valA = registers[rA];
//         long valB = registers[rB];
//         long valE = 0;
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

long readDouble(byte* readFrom, long loc){
    long r = 0;
    for(int i = loc+8-1; i >= loc; i--){
        r *= 2;
        r += (long) readFrom[i];
    }
    return r;
}

void writeDouble(byte *writeTo, long loc, long val){
    for(int i = 0; i < 8; i++){
        writeTo[i + loc] = (byte) (((val) & (0xFF << (i * 8))) >> (i * 8));
    }
}

long opQ(long rA, long rB, long fnCode){
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