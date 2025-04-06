#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <functional>

#include <emscripten/bind.h>
#include <emscripten/val.h>

const int canvasW = 1600;
const int canvasH = 900;

float rotation = 0;

void rotateAt(emscripten::val ctx, int x, int y, float angle){
    ctx.call<void>("translate", x, y);
    ctx.call<void>("rotate", angle);
    ctx.call<void>("translate", -x, -y);
}

std::string num2str(int64_t w, size_t hex_len = sizeof(int64_t)<<1) {
    static const char* digits = "0123456789ABCDEF";
    std::string rc(hex_len,'0');
    for (size_t i=0, j=(hex_len-1)*4 ; i<hex_len; ++i,j-=4)
        rc[i] = digits[(w>>j) & 0x0f];

    cout << rc << "\n";
    bool allZeroes = true;
    int firstNonzeroIndex = 0;
    for(int i = 0; i < hex_len; i++){
        if(rc[i] != '0'){
            firstNonzeroIndex = i;
            allZeroes = false;
            break;
        }
    }
    if(allZeroes == true) {
        return "0";
    }
    return rc.substr(firstNonzeroIndex);
}

const int registerW = 150;
const int registerH = canvasH / 15;
const int registerPadding = 5;
const int memW = 150 + registerPadding;// width for the memory
void renderRegister(emscripten::val ctx, int x, int y, string name, string val){//int64_t registerValue
    ctx.set("strokeStyle", "black");
    ctx.set("lineWidth", 2);
    ctx.call<void>("strokeRect", x + registerPadding, y + registerPadding, registerW - registerPadding*2, registerH - registerPadding*2);

    // string str = int64_t2str()
    ctx.set("fillStyle", "black");
    if(name.length() > 5) ctx.set("font", "600 13px Ubuntu");
    else ctx.set("font", "600 28px Ubuntu");
    ctx.set("textAlign", "center");
    ctx.set("textBaseline", "middle");
    
    ctx.call<void>("fillText", name, x + registerW/2, y + registerH/2 - 10);

    ctx.set("font", "600 15px Ubuntu");
    ctx.set("fillStyle", "black");
    ctx.call<void>("fillText", val, x + registerW/2, y + registerH/2 + 14);
}



void renderRegisters(emscripten::val ctx, int64_t* registers){
    const string registerNames[] = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14"};

    int x = canvasW/2 - registerW/2;
    int y = 0;
    for(int i = 0; i < 15; i++){
        renderRegister(ctx, x, y, registerNames[i], num2str(registers[i]));
        y += registerH;
    }
}

void renderConditionCode(emscripten::val ctx, int x, int y, string codeName, bool codeValue){
    renderRegister(ctx, x, y, codeName, codeValue == true ? "true" : "false");
}

void renderPC(emscripten::val ctx, int64_t val){
    renderRegister(ctx, 0, canvasH/2 - registerH/2, "PC", num2str(val));
}

void renderStatus(emscripten::val ctx, int x, int y, int status){
    string statusCode = "AOK";
    switch(status){
        case 2:
            statusCode = "HLT";
        case 3:
            statusCode = "ADR";
        case 4:
            statusCode = "INS";
    }
    renderRegister(ctx, x, y, "Status Code", statusCode);
}

int64_t readDouble(byte* readFrom, int64_t loc){
    int64_t r = 0;
    for(int i = loc+8-1; i >= loc; i--){
        r *= 1 << 8;
        r += (int64_t) readFrom[i];
    }
    return r;
}

void renderStackTop(emscripten::val ctx, int64_t* registers, byte* memoryData){
    int stackPtr = registers[4];
    
    int start = stackPtr - 15 * 8;
    if(start < 0) start = 0;
    int x = canvasW - memW;
    int y = 0;

    ctx.set("strokeStyle", "black");
    ctx.call<void>("beginPath");
    ctx.call<void>("moveTo", x, 0);
    ctx.call<void>("lineTo", x, canvasH);
    ctx.call<void>("stroke");
    ctx.call<void>("closePath");

    for(int i = start; i <= stackPtr; i+=8){
        y += registerH;
        renderRegister(ctx, x, y, "M[" + num2str(i) + "]", num2str((int64_t) readDouble(memoryData, i)));
    }
}

std::unordered_map<int, std::string> stepToStr;

void initRender(){
    stepToStr[0] = "Fetch";
    stepToStr[1] = "Decode";
    stepToStr[2] = "Execute";
    stepToStr[3] = "Memory";
    stepToStr[4] = "Write Back";
    stepToStr[5] = "PC";
}

void renderStage(emscripten::val ctx, string stage){
    // text
    ctx.set("textAlign", "right");
    ctx.set("textBaseline", "top");
    ctx.set("fillStyle", "black");
    ctx.set("font", "600 48px Ubuntu");
    ctx.call<void>("fillText", stage, canvasW - memW - 10, 10);
}

string keysToCheck[] = {"rA","rB","valA","valB","valC","valE","valP", "instructionCode","fnCode","condition","stackPtr"};
void render(std::unordered_map<string, int64_t> packagedValues, int64_t* registers, int stepNum, byte* memoryData, int pc, bool zeroFlag, bool overflowFlag, bool signedFlag) {// deltaTime, use for animations
    const auto document = emscripten::val::global("document");
    const auto canvas = document.call<emscripten::val, std::string>("querySelector", "canvas");

    auto ctx = canvas.call<emscripten::val, std::string>("getContext", "2d");

    // render background
    ctx.set("fillStyle", "white");
    ctx.call<void>("fillRect", 0, 0, canvasW, canvasH);

    renderRegisters(ctx, registers);
    renderPC(ctx, pc);
    renderStage(ctx, stepToStr[stepNum]);
    renderStackTop(ctx, registers, memoryData);
    renderConditionCode(ctx, 0,0,"Zero Flag", zeroFlag);
    renderConditionCode(ctx, 0,registerH,"Overflow Flag", overflowFlag);
    renderConditionCode(ctx, 0,2*registerH,"Signed Flag", signedFlag);
    
    int priorDisp = 0;
    for(int i = 0; i < 11; i++){
        //if(packagedValues.count(keysToCheck[i])) continue;

        int64_t val = packagedValues[keysToCheck[i]];
        string toRender = num2str(val);
        if(keysToCheck[i] == "condition") toRender = (val == (int64_t) 1 ? "true" : "false");

        cout << "key \n" << keysToCheck[i] << "\n text to render: \n" << toRender << "\n";

        renderRegister(ctx, registerW, priorDisp*registerH, keysToCheck[i], toRender);
        
        priorDisp++;
    }

    // my_map.contains(k1)

    // returnDict["rA"] = (int64_t) rA; //Casting to int64_t is OK here, we can just cast back to int later
    // returnDict["rB"] = (int64_t) rB;
    // returnDict["fnCode"] = (int64_t) fnCode
    // returnDict["valC"] = valC;
    // returnDict["valP"] = valB;
    // returnDict["instructionCode"] = (int64_t) instructionCode;
}

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
// ctx.set("font", "600 48px Ubuntu");
// ctx.call<void, std::string>("fillText", "Hello World!", width / 2, height / 2);




// ctx.call<void>("beginPath");
// ctx.set("strokeStyle", "red");
// ctx.call<void>("moveTo", x, y);
// ctx.call<void>("lineTo", x + registerW, y + registerH);
// ctx.call<void>("stroke");
// ctx.call<void>("closePath");

// EMSCRIPTEN_BINDINGS(my_module) {
//     emscripten::function("render", &render);
// }