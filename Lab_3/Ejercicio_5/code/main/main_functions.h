#ifndef MAIN_FUNCTIONS_H
#define MAIN_FUNCTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

void setup(void);
void loop(void);

// For testing or manual simulation run
void run_inference(void *ptr);

#ifdef __cplusplus
}
#endif

#endif // MAIN_FUNCTIONS_H
