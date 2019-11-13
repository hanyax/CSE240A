//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
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
uint32_t ghr32_tournament; // gobal history register

uint8_t* LPT_tournament; // local prediction
uint8_t* GPT_tournament; // global prediction
uint8_t* CPT_tournament; // choice prediction

int LHT_tournament_length;
int GPT_tournament_length;
int LPT_tournament_length;

/*
For Custom
*/
int ghistoryBits_bi = 13; // Number of bits used for Global History
int lhistoryBits_bi = 11; // Number of bits used for Local History
int pcIndexBits_bi = 10;  // Number of bits used for PC index

uint32_t* LHT_bi;// local history table
uint32_t ghr32_bi; // gobal history register

uint8_t* GPT_choice; // local prediction
uint8_t* GPT_t;
uint8_t* GPT_nt;

uint8_t* LPT_bi; // global prediction
uint8_t* CPT_bi; // choice prediction

int LHT_bi_length;
int GPT_bi_length;
int LPT_bi_length;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void GSHARE_init() {
  // init BHT
  BHT_gshare_length = 2 << ghistoryBits;
  BHT_gshare = malloc(sizeof(uint8_t) * BHT_gshare_length);
  memset(BHT_gshare, 1, sizeof(uint8_t) * BHT_gshare_length);

  // init Global History Register
  ghr32_gshare = 0;
}

void TOURNAMENT_init() {
  LHT_tournament_length = 2 << pcIndexBits;
  LPT_tournament_length = 2 << lhistoryBits;
  GPT_tournament_length = 2 << ghistoryBits;

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
  LHT_bi_length = 2 << pcIndexBits_bi;
  LPT_bi_length = 2 << lhistoryBits_bi;
  GPT_bi_length = 2 << ghistoryBits_bi;

  LHT_bi = malloc(sizeof(uint32_t) * LHT_bi_length);
  LPT_bi = malloc(sizeof(uint8_t) * LPT_bi_length);

  GPT_choice = malloc(sizeof(uint8_t) * GPT_bi_length);
  GPT_t = malloc(sizeof(uint8_t) * GPT_bi_length);
  GPT_nt = malloc(sizeof(uint8_t) * GPT_bi_length);
  CPT_bi = malloc(sizeof(uint8_t) * GPT_bi_length);

  memset(LHT_bi, 0, sizeof(uint32_t) * LHT_bi_length);
  memset(LPT_bi, 1, sizeof(uint8_t) * LPT_bi_length);
  memset(GPT_choice, 1, sizeof(uint8_t) * GPT_bi_length);
  memset(GPT_t, 2, sizeof(uint8_t) * GPT_bi_length);
  memset(GPT_nt, 1, sizeof(uint8_t) * GPT_bi_length);
  memset(CPT_bi, 1, sizeof(uint8_t) * GPT_bi_length);

  ghr32_bi = 0;
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
  uint8_t globel_prediction = GPT_tournament[masked_ghr];
  uint8_t choice_prediction = CPT_tournament[masked_ghr];

  // Get local prediction
  // find the index of LHT by masking PC
  uint32_t pc_mask = (1 << pcIndexBits) - 1;
  uint32_t masked_pc = pc & pc_mask;
  uint32_t LPT_index = LHT_tournament[masked_pc];

  // find the index of LPT by masking LHT
  uint32_t LPT_mask = (1 << lhistoryBits) - 1;
  uint32_t masked_LPT_index = LPT_index & LPT_mask;
  uint8_t local_prediction = LPT_tournament[masked_LPT_index];

  if (choice_prediction < 2) {
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
  uint32_t global_mask = (1 << ghistoryBits_bi) - 1;
  uint32_t global_masked_pc = pc & global_mask;
  uint32_t masked_ghr = (ghr32_bi^pc) & global_mask;
  uint8_t GPT_choice_prediction = GPT_choice[global_masked_pc];
  uint8_t GPT_t_prediction = GPT_t[masked_ghr];
  uint8_t GPT_nt_prediction = GPT_nt[masked_ghr];
  uint8_t globel_prediction;

  if (GPT_choice_prediction > 1) {
    if (GPT_t_prediction > 1) {
      globel_prediction = 1;
    } else {
      globel_prediction = 0;
    }
  } else {
    if (GPT_nt_prediction > 1) {
      globel_prediction = 1;
    } else {
      globel_prediction = 0;
    }
  }

  // choice
  uint8_t choice_prediction = CPT_bi[masked_ghr];

  // Get local prediction
  // find the index of LHT by masking PC
  uint32_t pc_mask = (1 << pcIndexBits_bi) - 1;
  uint32_t masked_pc = pc & pc_mask;
  uint32_t LPT_index = LHT_bi[masked_pc];

  // find the index of LPT by masking LHT
  uint32_t LPT_mask = (1 << lhistoryBits_bi) - 1;
  uint32_t masked_LPT_index = LPT_index & LPT_mask;
  uint8_t local_prediction = LPT_bi[masked_LPT_index];

  if (choice_prediction < 2) {
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
// outcome 'outcome' (truebi indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void train_predictor_GSHARE(uint32_t pc, uint8_t outcome) {
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

void increase_local_prediction(int index) {
  if (LPT_tournament[index] == 0) {
    LPT_tournament[index] = 1;
  } else if (LPT_tournament[index] == 1) {
    LPT_tournament[index] = 2;
  } else if (LPT_tournament[index] == 2) {
    LPT_tournament[index] = 3;
  } else {
    LPT_tournament[index] = 3;
  }
}

void decrease_local_prediction(int index) {
  if (LPT_tournament[index] == 0) {
    LPT_tournament[index] = 0;
  } else if (LPT_tournament[index] == 1) {
    LPT_tournament[index] = 0;
  } else if (LPT_tournament[index] == 2) {
    LPT_tournament[index] = 1;
  } else {
    LPT_tournament[index] = 2;
  }
}

void increase_global_prediction(int index) {
  if (GPT_tournament[index] == 0) {
    GPT_tournament[index] = 1;
  } else if (GPT_tournament[index] == 1) {
    GPT_tournament[index] = 2;
  } else if (GPT_tournament[index] == 2) {
    GPT_tournament[index] = 3;
  } else {
    GPT_tournament[index] = 3;
  }
}

void decrease_global_prediction(int index) {
  if (GPT_tournament[index] == 0) {
    GPT_tournament[index] = 0;
  } else if (GPT_tournament[index] == 1) {
    GPT_tournament[index] = 0;
  } else if (GPT_tournament[index] == 2) {
    GPT_tournament[index] = 1;
  } else {
    GPT_tournament[index] = 2;
  }
}

void increase_choice_prediction(int index) {
  if (CPT_tournament[index] == 0) {
    CPT_tournament[index] = 1;
  } else if (CPT_tournament[index] == 1) {
    CPT_tournament[index] = 2;
  } else if (CPT_tournament[index] == 2) {
    CPT_tournament[index] = 3;
  } else {
    CPT_tournament[index] = 3;
  }
}

void decrease_choice_prediction(int index) {
  if (CPT_tournament[index] == 0) {
    CPT_tournament[index] = 0;
  } else if (CPT_tournament[index] == 1) {
    CPT_tournament[index] = 0;
  } else if (CPT_tournament[index] == 2) {
    CPT_tournament[index] = 1;
  } else {
    CPT_tournament[index] = 2;
  }
}

void train_predictor_TOURNAMENT(uint32_t pc, uint8_t outcome) {
  uint32_t global_mask = (1 << ghistoryBits) - 1;
  uint32_t masked_ghr = ghr32_tournament & global_mask;
  uint8_t globel_prediction = GPT_tournament[masked_ghr];
  uint8_t choice_prediction = CPT_tournament[masked_ghr];

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
  LHT_tournament[masked_pc] = LHT_tournament[masked_pc] << 1;
  // Index into BHT and update BHT
  if (outcome == 1) {
    increase_local_prediction(masked_LPT_index);
    increase_global_prediction(masked_ghr);

    if (choice_prediction < 2) { // predict global
      if (globel_prediction < 2) { // predict wrong
        if (local_prediction > 2) {
          increase_choice_prediction(masked_ghr);
        }
      } else { // predict right
        if (local_prediction < 2) {
          decrease_choice_prediction(masked_ghr);
        }
      }
    } else { // predict local
      if (local_prediction < 2) { // predict wrong
        if (globel_prediction > 2) {
          decrease_choice_prediction(masked_ghr);
        }
      } else {
        if (globel_prediction < 2) {
          increase_choice_prediction(masked_ghr);
        }
      }
    }

    /////////// update LHT ////////////////////
    LHT_tournament[masked_pc] |= 1 << 0; // set local history last bit to 1
    ghr32_tournament |= 1 << 0; // set global history last bit to 1

  } else { // NOTTAKEN
    decrease_local_prediction(masked_LPT_index);
    decrease_global_prediction(masked_ghr);

    if (choice_prediction < 2) { // predict global
      if (globel_prediction < 2) { // predict right
        if (local_prediction > 2) {
          decrease_choice_prediction(masked_ghr);
        }
      } else {
        if (local_prediction < 2) {
          increase_choice_prediction(masked_ghr);
        }
      }
    } else { // predict local
      if (local_prediction < 2) { // predict right
        if (globel_prediction > 2) {
          increase_choice_prediction(masked_ghr);
        }
      } else {
        if (globel_prediction < 2) {
          decrease_choice_prediction(masked_ghr);
        }
      }
    }

    /////////// update LHT ////////////////////
    LHT_tournament[masked_pc] &= ~(1 << 0); // set local history last bit to 0
    ghr32_tournament &= ~(1 << 0); // set global history last bit to 0
  }
}

void increase_local_prediction_bi(int index) {
  if (LPT_bi[index] == 0) {
    LPT_bi[index] = 1;
  } else if (LPT_bi[index] == 1) {
    LPT_bi[index] = 2;
  } else if (LPT_bi[index] == 2) {
    LPT_bi[index] = 3;
  } else {
    LPT_bi[index] = 3;
  }
}

void decrease_local_prediction_bi(int index) {
  if (LPT_bi[index] == 0) {
    LPT_bi[index] = 0;
  } else if (LPT_bi[index] == 1) {
    LPT_bi[index] = 0;
  } else if (LPT_bi[index] == 2) {
    LPT_bi[index] = 1;
  } else {
    LPT_bi[index] = 2;
  }
}

void increase_choice_prediction_bi(int index) {
  if (CPT_bi[index] == 0) {
    CPT_bi[index] = 1;
  } else if (CPT_bi[index] == 1) {
    CPT_bi[index] = 2;
  } else if (CPT_bi[index] == 2) {
    CPT_bi[index] = 3;
  } else {
    CPT_bi[index] = 3;
  }
}

void decrease_choice_prediction_bi(int index) {
  if (CPT_bi[index] == 0) {
    CPT_bi[index] = 0;
  } else if (CPT_bi[index] == 1) {
    CPT_bi[index] = 0;
  } else if (CPT_bi[index] == 2) {
    CPT_bi[index] = 1;
  } else {
    CPT_bi[index] = 2;
  }
}

void update_global_prediction(int outcome, int masked_ghr, int global_masked_pc, int globel_prediction, int GPT_choice_prediction) {
  if (outcome == 1) {
    if (GPT_choice_prediction > 1) { // choose t
      if (GPT_t[masked_ghr] < 3) {
        GPT_t[masked_ghr] += 1;
      }
    } else { // choose nt
      if (GPT_nt[masked_ghr] < 3) {
        GPT_nt[masked_ghr] += 1;
      }
    }
  } else {
    if (GPT_choice_prediction > 1) { // choose t
      if (GPT_t[masked_ghr] > 0) {
        GPT_t[masked_ghr] -= 1;
      }
    } else { // choose nt
      if (GPT_nt[masked_ghr] > 0) {
        GPT_nt[masked_ghr] -= 1;
      }
    }
  }

  // update choice
  if (!((outcome == 0 && GPT_choice[masked_ghr] > 1 && globel_prediction==0) || (outcome == 1 && GPT_choice[masked_ghr] < 2 && globel_prediction==1))) {
    if ((outcome == 0 && globel_prediction == 0) || (outcome == 1 && globel_prediction == 1)) {
      if (GPT_choice[global_masked_pc] > 1) {
        if (GPT_choice[global_masked_pc] < 3) {
          GPT_choice[global_masked_pc] += 1;
        }
      } else {
        if (GPT_choice[global_masked_pc] > 0) {
          GPT_choice[global_masked_pc] -= 1;
        }
      }
    } else {
      if (GPT_choice[global_masked_pc] > 1) {
        if (GPT_choice[global_masked_pc] > 0) {
          GPT_choice[global_masked_pc] -= 1;
        }
      } else {
        if (GPT_choice[global_masked_pc] < 3) {
          GPT_choice[global_masked_pc] += 1;
        }
      }
    }
  }
}

void train_predictor_CUSTOM(uint32_t pc, uint8_t outcome) {
  uint32_t global_mask = (1 << ghistoryBits_bi) - 1;
  uint32_t global_masked_pc = pc & global_mask;
  uint32_t masked_ghr = (ghr32_bi ^ pc) & global_mask;
  uint8_t GPT_choice_prediction = GPT_choice[global_masked_pc];
  uint8_t GPT_t_prediction = GPT_t[masked_ghr];
  uint8_t GPT_nt_prediction = GPT_nt[masked_ghr];
  uint8_t globel_prediction;

  if (GPT_choice_prediction > 1) {
    if (GPT_t_prediction > 1) {
      globel_prediction = 1;
    } else {
      globel_prediction = 0;
    }
  } else {
    if (GPT_nt_prediction > 1) {
      globel_prediction = 1;
    } else {
      globel_prediction = 0;
    }
  }

  // choice
  uint8_t choice_prediction = CPT_bi[masked_ghr];

  // Get local prediction
  // find the index of LHT by masking PC
  uint32_t pc_mask = (1 << pcIndexBits_bi) - 1;
  uint32_t masked_pc = pc & pc_mask;
  uint32_t LPT_index = LHT_bi[masked_pc];

  // find the index of LPT by masking LHT
  uint32_t LPT_mask = (1 << lhistoryBits_bi) - 1;
  uint32_t masked_LPT_index = LPT_index & LPT_mask;
  uint8_t local_prediction = LPT_bi[masked_LPT_index];

  ghr32_bi = ghr32_bi << 1; // shift global history by 1
  LHT_bi[masked_pc] = LHT_bi[masked_pc] << 1;
  // Index into BHT and update BHT
  if (outcome == 1) {
    increase_local_prediction_bi(masked_LPT_index);
    update_global_prediction(outcome, masked_ghr, global_masked_pc, globel_prediction, GPT_choice_prediction);

    if (choice_prediction < 2) { // predict global
      if (globel_prediction < 2) { // predict wrong
        if (local_prediction > 1) {
          increase_choice_prediction_bi(masked_ghr);
        }
      } else { // predict right
        if (local_prediction < 2) {
          decrease_choice_prediction_bi(masked_ghr);
        }
      }
    } else { // predict local
      if (local_prediction < 2) { // predict wrong
        if (globel_prediction > 1) {
          decrease_choice_prediction_bi(masked_ghr);
        }
      } else {
        if (globel_prediction < 2) {
          increase_choice_prediction_bi(masked_ghr);
        }
      }
    }

    /////////// update LHT ////////////////////
    LHT_bi[masked_pc] |= 1 << 0; // set local history last bit to 1
    ghr32_bi |= 1 << 0; // set global history last bit to 1

  } else { // NOTTAKEN
    decrease_local_prediction_bi(masked_LPT_index);
    update_global_prediction(outcome, masked_ghr, global_masked_pc, globel_prediction, GPT_choice_prediction);

    if (choice_prediction < 2) { // predict global
      if (globel_prediction < 2) { // predict right
        if (local_prediction > 1) {
          decrease_choice_prediction_bi(masked_ghr);
        }
      } else {
        if (local_prediction < 2) {
          increase_choice_prediction_bi(masked_ghr);
        }
      }
    } else { // predict local
      if (local_prediction < 2) { // predict right
        if (globel_prediction > 1) {
          increase_choice_prediction_bi(masked_ghr);
        }
      } else {
        if (globel_prediction < 2) {
          decrease_choice_prediction_bi(masked_ghr);
        }
      }
    }

    /////////// update LHT ////////////////////
    LHT_bi[masked_pc] &= ~(1 << 0); // set local history last bit to 0
    ghr32_bi &= ~(1 << 0); // set global history last bit to 0
  }
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
