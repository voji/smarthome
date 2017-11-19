//debug routines
#define DEBUG_PRINTER Serial 

#ifdef DEBUG
  #define DP(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
  #define DPN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#else
  #define DP(...) {}
  #define DPN(...) {}
#endif  