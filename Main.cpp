#include <iostream>
#include <thread>
#include <unordered_map>
#include <string>
#include <random>
#include <ctime>
#ifdef _WIN32
#include <Windows.h>
#else
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <unistd.h>
#endif




class BaseOption
{
public:
    inline BaseOption(const size_t maxVals, const bool necessary) : maximumValues(maxVals), isNecessary(necessary)
    {
        values = nullptr;
    }
    inline virtual ~BaseOption() { if (values) { delete[] values; }};
    inline virtual size_t getMaxValues() const { return maximumValues; }
    inline virtual size_t getAmoutOfValues() const { return valuesGot; }
    inline virtual void addValue(char* val) = 0;
    inline virtual void* getValues() const { return values; }
    inline virtual bool isUsed() const { return wasItterated; }
    inline virtual void use() { wasItterated = true; }
    inline virtual bool necessary() const { return isNecessary; }
protected:
    const size_t maximumValues;
    const bool isNecessary;
    bool wasItterated = false;

    void* values;
    size_t valuesGot=0;

};  
class IntOption: public BaseOption
{
public:
    inline IntOption(const size_t maxVals, const bool necessary,int* defaultVals) : BaseOption(maxVals, necessary)
    {
        if (!defaultVals || _msize(defaultVals) / sizeof(*defaultVals) != maxVals) throw std::runtime_error("Standart agrument amount didn't met with given amount of values needed");
        values = defaultVals;
        buffer = static_cast<int*>(values);
    };
    inline virtual void  addValue(char* val) override
    {
        if (valuesGot == maximumValues) throw std::invalid_argument("Bad value count");
        buffer[valuesGot++] = std::stoi(val);
    }
private:
    int* buffer;
};
std::unordered_map<std::string, BaseOption*> settings
{
    {"chance",  new IntOption(1,false,new int {50})},
    {"seed",    new IntOption(1,false,new int{(int)time(0)})},
    {"delay",   new IntOption(1,false,new int{50})}
};



inline void PrintHelp()
{
    std::cout <<
        "--chance<n>|-c     n will be used as chance of uppercase\n"
        "--seed<n>|-s       n will be used as a seed for randomization\n"
        "--delay<n>|-d      n will be used as a delay between randomization\n"
        "--help|-h          get help\n";
    exit(1);
}


inline void getValues(const char* key,char** argv, size_t& index)
{ 
    if (settings.find(key) == settings.end()) throw std::invalid_argument("Unknown argument " + std::string(key));
    size_t i;
    BaseOption* option = settings[key];
    size_t shift=option->getMaxValues();
    option->use();
    if (!shift) return;
    for (i = 0; i < shift; ++i) 
    {
        option->addValue(argv[++index]);
    }
}


inline void processArgs(size_t argc, char** argv)
{
    size_t i;
    std::string arg;
    for (i = 1; i < argc; ++i)
    {   
        arg = argv[i];
        if(arg == "--help" || arg == "-h")
        {
            PrintHelp();
        }
        else if(arg == "--chance" || arg == "-c")
        {
            getValues("chance", argv, i);
        }
        else if (arg == "--seed" || arg == "-s")
        {
            getValues("seed", argv, i);
        }
        else if (arg == "--delay" || arg == "-d")
        {
            getValues("delay", argv, i);
        }
        else
        {
            std::cout << arg << ".\n";
            std::cout << "Use --help for help";
            throw std::invalid_argument("Unknown argument \""+ arg +"\"");
        }
    }
    for (const auto& p : settings)
    {
        if (!p.second->isUsed() && p.second->necessary())
            throw std::invalid_argument("Didn't get necessary argument --" + p.first);
    }
}
void freeMemory() 
{
    for (const auto& p : settings)
    {
        delete p.second;
    }    
}
#ifdef _WIN32
void backgroundFunc(bool* safeAndPause)
{
    int chance,random;

    while (safeAndPause[0])
    {
        chance = *(int*)settings["chance"]->getValues();
        random = rand() % 100;

        if(chance >= random && !safeAndPause[1])
        {
#ifdef _DEBUG
            std::cout << "pressed\n";
#endif // _DEBUG
            keybd_event(VK_SHIFT, 0, KEYEVENTF_EXTENDEDKEY | 0, 0);
            Sleep(*(int*)settings["delay"]->getValues());
            keybd_event(VK_SHIFT, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
#ifdef _DEBUG
            std::cout << "released\n";
#endif // _DEBUG
        }
        else
        {
            Sleep(*(int*)settings["delay"]->getValues());
        }
    }
#ifdef _DEBUG
    std::cout << "Thread ended\n";
#endif // _DEBUG
    safeAndPause[0] = true;
}
#else
void backgroundFunc(bool* safeAndPause,Display* display)
{
    int chance, random;

    while (safeAndPause[0])
    {
        if (chance >= random)
        {
            XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Shift_L), True, 0);
            XFlush(display);
            sleep(*(int*)settings["delay"]->getValues() / 1000);
            XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Shift_L), False, 0);
            XFlush(display);
        }
        else
        {
            sleep(*(int*)settings["delay"]->getValues() / 1000);
        }
    }
    XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Shift_L), False, 0);
    XFlush(display);
    safeAndPause[0] = true;
}
#endif

int main(size_t argc, char** argv)
{
    srand(*(int*)settings["seed"]->getValues());
    processArgs(argc, argv);
    bool* safe = new bool[2] {true, false};
#ifdef _WIN32
#ifndef _DEBUG
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);
#endif
    std::thread bgProcess(backgroundFunc, safe);
#else
    Display* display = XOpenDisplay(NULL);
    std::thread bgProcess(backgroundFunc, safe, display);
    KeyCode alt = XKeysymToKeycode(display, XK_Alt_L);
    KeyCode stop = XKeysymToKeycode(display, XK_s);
    KeyCode pause = XKeysymToKeycode(display, XK_p);
    KeyCode stopCapital = XKeysymToKeycode(display, XK_S);
    KeyCode pauseCapital = XKeysymToKeycode(display, XK_P);
    XQueryKeymap(display, NULL);

#endif

    long long pausetime=-1;
    bgProcess.detach();
    while(safe[0])
    {

#ifdef _WIN32        
        if (GetAsyncKeyState(VK_LMENU) & 0x8000 && GetAsyncKeyState(0x53) & 0x8000) 
        {
            safe[0] = false;
        }
        else if (GetAsyncKeyState(VK_LMENU) & 0x8000 && GetAsyncKeyState(0x50) & 0x8000)
        {
#ifdef _DEBUG
            if (safe[1])
                std::cout << "Unpaused\n";
            else
                std::cout << "Paused\n";
#endif // _DEBUG
            safe[1] = !safe[1];
            Sleep(250);
        }
        Sleep(1);
#else
        char keymap[32];
        XQueryKeymap(display, keymap);

        bool isAltPressed = (keymap[alt / 8] & (1 << (alt % 8))) != 0;
        bool isStopPressed = (keymap[stop / 8] & (1 << (stop % 8))) != 0;
        bool isPausePressed = (keymap[pause / 8] & (1 << (pause % 8))) != 0;
        bool isStopCapitalPressed = (keymap[stopCapital / 8] & (1 << (stopCapital % 8))) != 0;
        bool isPauseCapitalPressed = (keymap[pauseCapital / 8] & (1 << (pauseCapital % 8))) != 0;
        if (isAltPressed&&(isStopPressed||isStopCapitalPressed)) {
            safe[0] = false;
        }
        else if (isAltPressed && (isPausePressed || isPauseCapitalPressed)) {
            safe[1] = !safe[1];
            usleep(250000);
        }
        usleep(1000);
#endif
    }
    while (!safe[0]) {
#ifdef _WIN32
        Sleep(1);
#else
        usleep(1000);
#endif
    }
#ifndef _WIN32
    XCloseDisplay(display);
#endif
    freeMemory();
    return 0;

}
