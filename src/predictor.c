//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Hanyang Xu";
const char *studentID   = "A92068025";
const char *email       = "hax032@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//

/*
For GSHARE
*/
uint8_t* BHT_gshare;
int BHT_gshare_length;
uint32_t ghr32_gshare;

/*
For Tournament
*/
uint32_t* LHT_tournament;// local history table

uint32_t ghr32_tournament;
uint8_t* LPT_tournament; // local prediction
uint8_t* GPT_tournament; // global prediction
uint8_t* CPT_tournament; // choice prediction

int LHT_tournament_length;
int GPT_tournament_length;
int LPT_tournament_length;

/*
For Custom
*/

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

void binary_helper(unsigned number, int bit) {
  for (int i = 0; i < bit; ++i) {
    if (number >> i & 0x1) putchar('1');
    else putchar('0');
  }
  putchar('\n');
}

// Initialize the predictor
//
void GSHARE_init() {
  // init BHT
  BHT_gshare_length = (int) pow((double)2, (double)ghistoryBits);
  BHT_gshare = malloc(sizeof(uint8_t) * BHT_gshare_length);
  memset(BHT_gshare, 1, sizeof(uint8_t) * BHT_gshare_length);

  // init Global History Register
  ghr32_gshare = 0;
}

void TOURNAMENT_init() {
  LHT_tournament_length = (int) pow((double)2, (double)pcIndexBits);
  LPT_tournament_length = (int) pow((double)2, (double)lhistoryBits);
  GPT_tournament_length = (int) pow((double)2, (double)ghistoryBits);

  LHT_tournament = malloc(sizeof(uint32_t) * LHT_tournament_length);
  LPT_tournament = malloc(sizeof(uint8_t) * LPT_tournament_length);
  GPT_tournament = malloc(sizeof(uint8_t) * GPT_tournament_length);
  CPT_tournament = malloc(sizeof(uint8_t) * GPT_tournament_length);

  memset(LHT_tournament, 0, sizeof(uint32_t) * LHT_tournament_length);
  memset(LPT_tournament, 1, sizeof(uint8_t) * LPT_tournament_length);
  memset(GPT_tournament, 1, sizeof(uint8_t) * GPT_tournament_length);
  memset(CPT_tournament, 1, sizeof(uint8_t) * GPT_tournament_length); // 0, 1 is global 2, 3 is local

  ghr32_tournament = 0;
}

void CUSTOM_init() {

}

void init_predictor() {
  //
  //TODO: Initialize Branch Predictor Data Structures
  //
  GSHARE_init();
  TOURNAMENT_init();
  CUSTOM_init();
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t make_prediction_GSHARE(uint32_t pc) {
  // Compute Xor
  uint32_t mask = (1 << ghistoryBits) - 1;
  uint32_t masked_pc = pc & mask;
  uint32_t masked_ghr = ghr32_gshare & mask;
  int index = (int) (masked_pc ^ masked_ghr);
  uint8_t prediction = BHT_gshare[index];

  assert(prediction >= 0 && prediction < 4);

  if (prediction > 1) {
    return TAKEN;
  } else {
    return NOTTAKEN;
  }
}

uint8_t make_prediction_TOURNAMENT(uint32_t pc) {
  //printf("Tournament Predicting...\n");
  // Get global prediction
  uint32_t global_mask = (1 << ghistoryBits) - 1;
  uint32_t masked_ghr = ghr32_tournament & global_mask;
  int index = (int) masked_ghr;
  uint8_t globel_prediction = GPT_tournament[index];
  uint8_t choice_prediction = CPT_tournament[index];

  assert(choice_prediction>=0 && choice_prediction <= 3);
  assert(globel_prediction>=0 && globel_prediction <= 3);

  // Get local prediction
  // find the index of LHT by masking PC
  uint32_t pc_mask = (1 << pcIndexBits) - 1;
  uint32_t masked_pc = pc & pc_mask;
  uint32_t LPT_index = LHT_tournament[masked_pc];

  // find the index of LPT by masking LHT
  uint32_t LPT_mask = (1 << lhistoryBits) - 1;
  uint32_t masked_LPT_index = LPT_index & LPT_mask;
  uint8_t local_prediction = LPT_tournament[masked_LPT_index];

  if (choice_prediction <= 1) {
    if (globel_prediction > 1) {
      return TAKEN;
    } else {
      return NOTTAKEN;
    }
  } else {
    if (local_prediction > 1) {
      return TAKEN;
    } else {
      return NOTTAKEN;
    }
  }
}

uint8_t make_prediction_CUSTOM(uint32_t pc) {
  return NOTTAKEN;
}

uint8_t make_prediction(uint32_t pc) {
  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return make_prediction_GSHARE(pc);
    case TOURNAMENT:
      return make_prediction_TOURNAMENT(pc);
    case CUSTOM:
      return make_prediction_CUSTOM(pc);
    default:
      break;
}

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void  train_predictor_GSHARE(uint32_t pc, uint8_t outcome) {
    // Compute Xor
    uint32_t mask = (1 << ghistoryBits) - 1;
    uint32_t masked_pc = pc & mask;
    uint32_t masked_ghr = ghr32_gshare & mask;
    int index = masked_pc ^ masked_ghr;

    ghr32_gshare = ghr32_gshare << 1;

    int BHT_value = (int) BHT_gshare[index];
      assert(BHT_value >= 0 && BHT_value < 4);
    // Index into BHT and update BHT
    if (outcome == TAKEN) {
      if (BHT_value == 0) {
        BHT_gshare[index] = 1;
      } else if (BHT_value == 1) {
        BHT_gshare[index] = 2;
      } else if (BHT_value == 2) {
        BHT_gshare[index] = 3;
      } else if (BHT_value == 3) {
        BHT_gshare[index] = 3;
      } else {
        printf("I am here, sth is wrong\n");
      }
      ghr32_gshare |= 1 << 0; // set last bit to 1
    } else {
      if (BHT_value == 0) {
        BHT_gshare[index] = 0;
      } else if (BHT_value == 1) {
        BHT_gshare[index] = 0;
      } else if (BHT_value == 2) {
        BHT_gshare[index] = 1;
      } else if (BHT_value == 3) {
        BHT_gshare[index] = 2;
      } else {
        printf("I am here, sth is wrong\n");
      }
      ghr32_gshare &= ~(1 << 0); // set last bit to 0
    }
}

void  train_predictor_TOURNAMENT(uint32_t pc, uint8_t outcome) {
  //printf("Tournament Training...\n");
  uint32_t global_mask = (1 << ghistoryBits) - 1;
  uint32_t masked_ghr = ghr32_tournament & global_mask;
  int index = (int) masked_ghr;
  uint8_t globel_prediction = GPT_tournament[index];
  uint8_t choice_prediction = CPT_tournament[index];

  // Get local prediction
  // find the index of LHT by masking PC
  uint32_t pc_mask = (1 << pcIndexBits) - 1;
  uint32_t masked_pc = pc & pc_mask;
  uint32_t LPT_index = LHT_tournament[masked_pc];

  // find the index of LPT by masking LHT
  uint32_t LPT_mask = (1 << lhistoryBits) - 1;
  uint32_t masked_LPT_index = LPT_index & LPT_mask;
  uint8_t local_prediction = LPT_tournament[masked_LPT_index];

  ghr32_tournament = ghr32_tournament << 1; // shift global history by 1
  // Index into BHT and update BHT
  if (outcome == TAKEN) {
    if (choice_prediction <= 1) { // update global
      if (globel_prediction == 0) { // predict wrong, increase choice table
        GPT_tournament[index] = 1;

        if (choice_prediction == 0) {
          CPT_tournament[index] = 1;
        } else if (choice_prediction == 1) {
          CPT_tournament[index] = 2;
        } else {
          printf("I am here, sth is wrong in choice prediction value\n");
        }
      } else if (globel_prediction == 1) { // predict wrong, increase choice table
        GPT_tournament[index] = 2;

        if (choice_prediction == 0) {
          CPT_tournament[index] = 1;
        } else if (choice_prediction == 1) {
          CPT_tournament[index] = 2;
        } else {
          printf("sth is wrong in choice prediction value\n");
        }
      } else if (globel_prediction == 2) { // predict correct, decrease choice table
        GPT_tournament[index] = 3;

        if (choice_prediction == 0) {
          CPT_tournament[index] = 0;
        } else if (choice_prediction == 1) {
          CPT_tournament[index] = 0;
        } else {
          printf("sth is wrong in choice prediction value\n");
        }
      } else if (globel_prediction == 3) { // predict correct. decrease choice table
        GPT_tournament[index] = 3;

        if (choice_prediction == 0) {
          CPT_tournament[index] = 0;
        } else if (choice_prediction == 1) {
          CPT_tournament[index] = 0;
        } else {
          printf("sth is wrong in choice prediction value\n");
        }
      } else {
        printf("sth is wrong with globel_prediction value\n");
      }
    } else if (choice_prediction > 1) { // update local
      //////////// update LPT //////////////
      if (local_prediction == 0) { // predict wrong, decrease choice table
        LPT_tournament[masked_LPT_index] = 1;

        if (choice_prediction == 2) {
          CPT_tournament[index] = 1;
        } else if (choice_prediction == 3) {
          CPT_tournament[index] = 2;
        } else {
          printf("sth is wrong in choice prediction value\n");
        }
      } else if (local_prediction == 1) { // predict wrong, decrease choice table
        LPT_tournament[masked_LPT_index] = 2;

        if (choice_prediction == 2) {
          CPT_tournament[index] = 1;
        } else if (choice_prediction == 3) {
          CPT_tournament[index] = 2;
        } else {
          printf("sth is wrong in choice prediction value\n");
        }
      } else if (local_prediction == 2) { // predict right, increase choice table
        LPT_tournament[masked_LPT_index] = 3;

        if (choice_prediction == 2) {
          CPT_tournament[index] = 3;
        } else if (choice_prediction == 3) {
          CPT_tournament[index] = 3;
        } else {
          printf("sth is wrong in choice prediction value\n");
        }
      } else if (local_prediction == 3) { // predict right, increase choice table
        LPT_tournament[masked_LPT_index] = 3;

        if (choice_prediction == 2) {
          CPT_tournament[index] = 3;
        } else if (choice_prediction == 3) {
          CPT_tournament[index] = 3;
        } else {
          printf("sth is wrong in choice prediction value\n");
        }
      } else {
        printf("sth is wrong with local_prediction value\n");
      }

      /////////// update LHT ////////////////////
      LHT_tournament[masked_pc] = LHT_tournament[masked_pc]<< 1;
      LHT_tournament[masked_pc] |= 1 << 0; // set local history last bit to 1

    } else { //
      printf("sth is wrong with choice_prediction value\n");
    }

    ghr32_tournament |= 1 << 0; // set global history last bit to 1
  } else { // NOTTAKEN
    if (choice_prediction <= 1) { // update global
      if (globel_prediction == 0) { // predict correct, decrease choice table
        GPT_tournament[index] = 0;

        if (choice_prediction == 0) {
          CPT_tournament[index] = 0;
        } else if (choice_prediction == 1) {
          CPT_tournament[index] = 0;
        } else {
          printf("I am here, sth is wrong in choice prediction value\n");
        }
      } else if (globel_prediction == 1) { // predict correct, decrease choice table
        GPT_tournament[index] = 0;

        if (choice_prediction == 0) {
          CPT_tournament[index] = 0;
        } else if (choice_prediction == 1) {
          CPT_tournament[index] = 0;
        } else {
          printf("sth is wrong in choice prediction value\n");
        }
      } else if (globel_prediction == 2) { // predict wrong, increase choice table
        GPT_tournament[index] = 1;

        if (choice_prediction == 0) {
          CPT_tournament[index] = 1;
        } else if (choice_prediction == 1) {
          CPT_tournament[index] = 2;
        } else {
          printf("sth is wrong in choice prediction value\n");
        }
      } else if (globel_prediction == 3) { // predict wrong. increase choice table
        GPT_tournament[index] = 2;

        if (choice_prediction == 0) {
          CPT_tournament[index] = 1;
        } else if (choice_prediction == 1) {
          CPT_tournament[index] = 2;
        } else {
          printf("sth is wrong in choice prediction value\n");
        }
      } else {
        printf("sth is wrong with globel_prediction value\n");
      }
    } else if (choice_prediction > 1) { // update local
      //////////// update LPT //////////////
      if (local_prediction == 0) { // predict right, increase choice table
        LPT_tournament[masked_LPT_index] = 0;

        if (choice_prediction == 2) {
          CPT_tournament[index] = 3;
        } else if (choice_prediction == 3) {
          CPT_tournament[index] = 3;
        } else {
          printf("sth is wrong in choice prediction value\n");
        }
      } else if (local_prediction == 1) { // predict right, increase choice table
        LPT_tournament[masked_LPT_index] = 0;

        if (choice_prediction == 2) {
          CPT_tournament[index] = 3;
        } else if (choice_prediction == 3) {
          CPT_tournament[index] = 3;
        } else {
          printf("sth is wrong in choice prediction value\n");
        }
      } else if (local_prediction == 2) { // predict wrong, decrease choice table
        LPT_tournament[masked_LPT_index] = 1;

        if (choice_prediction == 2) {
          CPT_tournament[index] = 1;
        } else if (choice_prediction == 3) {
          CPT_tournament[index] = 2;
        } else {
          printf("sth is wrong in choice prediction value\n");
        }
      } else if (local_prediction == 3) { // predict wrong, decrease choice table
        LPT_tournament[masked_LPT_index] = 2;

        if (choice_prediction == 2) {
          CPT_tournament[index] = 1;
        } else if (choice_prediction == 3) {
          CPT_tournament[index] = 2;
        } else {
          printf("sth is wrong in choice prediction value\n");
        }
      } else {
        printf("sth is wrong with local_prediction value\n");
      }

      /////////// update LHT ////////////////////
      LHT_tournament[masked_pc] = LHT_tournament[masked_pc]<< 1;
      LHT_tournament[masked_pc] &= ~(1 << 0); // set local history last bit to 0

    } else { //
      printf("sth is wrong with choice_prediction value\n");
    }

    ghr32_tournament &= ~(1 << 0); // set global history last bit to 0
  }
}

void  train_predictor_CUSTOM(uint32_t pc, uint8_t outcome) {

}

void train_predictor(uint32_t pc, uint8_t outcome) {
  // Make a prediction based on the bpType
  switch (bpType) {
    case GSHARE:
      train_predictor_GSHARE(pc, outcome);
    case TOURNAMENT:
      train_predictor_TOURNAMENT(pc, outcome);
    case CUSTOM:
      train_predictor_CUSTOM(pc, outcome);
    default:
      break;
  }
}
