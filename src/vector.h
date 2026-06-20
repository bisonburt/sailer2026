/*
 *   File     : vector.h
 *   Author   : Sean Graves
 *
 *   Computer : Any 
 *   Date     : 6/21/90 
 *
 *   Header for vector.c.  Should be included in any file that
 *   calls any functions in vector.c 
 *
 *
 * Copyright (c) 1990, by Sean Graves and Texas A&M University
 *
 * Permission is hereby granted for non-commercial reproduction and use of
 * this program, provided that this notice is included in any material copied
 * from it. The author assumes no responsibility for damages resulting from
 * the use of this software, however caused.
 *
 */

/********************* Function Prototypes *********************/

void mmult4x4_4x4();
void mmult1x4_4x1();
void mmult4x4_4x1();
void mmult1x4_4x4();
void transpose();
int fcmp();
float length();
float magnitude();
void normalize();
void cross_product();
float dot_product();
