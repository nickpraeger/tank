#pragma once

#include "pebble.h"


static const GPathInfo MINUTE_HAND_POINTS = {
  5, (GPoint []) {
    {-4,   0},
    { 4,   0},
    { 5, -53},
    { 0, -60 },
    {-5, -53 }
  }
};

static const GPathInfo HOUR_HAND_POINTS = {
  5, (GPoint []){
    {-3,  0},
    { 3,  0}, 
    { 5,-33},
    { 0,-40},
    {-5,-33}
  }
};

static const GPathInfo BATT_HAND_POINTS = {
  5, (GPoint []) {
    {-1,  0},
    { 1,  0},
    { 2,-18},
    { 0,-20 },
    {-2,-18 }
  }
};