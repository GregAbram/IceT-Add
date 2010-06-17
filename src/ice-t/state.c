/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2003 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the U.S. Government.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this Notice and any statement
 * of authorship are reproduced on all copies.
 */

#include <state.h>

#include <IceT.h>
#include <context.h>
#include <diagnostics.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int typeWidth(IceTEnum type);
static void stateSet(IceTEnum pname, IceTInt size, IceTEnum type,
                     const IceTVoid *data);

IceTState icetStateCreate(void)
{
    IceTState state;

    state = (IceTState)malloc(sizeof(struct IceTStateValue) * ICET_STATE_SIZE);
    memset(state, 0, sizeof(struct IceTStateValue) * ICET_STATE_SIZE);

    return state;
}

void icetStateDestroy(IceTState state)
{
    IceTEnum i;

    for (i = 0; i < ICET_STATE_SIZE; i++) {
        if (state[i].type != ICET_NULL) {
            free(state[i].data);
        }
    }
    free(state);
}

void icetStateCopy(IceTState dest, const IceTState src)
{
    IceTEnum i;
    int type_width;
    IceTTimeStamp mod_time;

    mod_time = icetGetTimeStamp();

    for (i = 0; i < ICET_STATE_SIZE; i++) {
        if (   (i == ICET_RANK) || (i == ICET_NUM_PROCESSES)
            || (i == ICET_DATA_REPLICATION_GROUP)
            || (i == ICET_DATA_REPLICATION_GROUP_SIZE)
            || (i == ICET_COMPOSITE_ORDER) || (i == ICET_PROCESS_ORDERS) )
        {
            continue;
        }

        if (dest[i].type != ICET_NULL) {
            free(dest[i].data);
        }

        type_width = typeWidth(src[i].type);

        dest[i].type = src[i].type;
        dest[i].size = src[i].size;
        if (type_width > 0) {
            dest[i].data = malloc(type_width * dest[i].size);
            memcpy(dest[i].data, src[i].data, src[i].size * type_width);
        } else {
            dest[i].data = NULL;
        }
        dest[i].mod_time = mod_time;
    }
}

static IceTFloat black[] = {0.0, 0.0, 0.0, 0.0};

void icetStateSetDefaults(void)
{
    IceTInt *int_array;
    int i;

    icetDiagnostics(ICET_DIAG_ALL_NODES | ICET_DIAG_WARNINGS);

    icetStateSetInteger(ICET_RANK, ICET_COMM_RANK());
    icetStateSetInteger(ICET_NUM_PROCESSES, ICET_COMM_SIZE());
    /* icetStateSetInteger(ICET_ABSOLUTE_FAR_DEPTH, 1); */
  /*icetStateSetInteger(ICET_ABSOLUTE_FAR_DEPTH, 0xFFFFFFFF);*/
    icetStateSetFloatv(ICET_BACKGROUND_COLOR, 4, black);
    icetStateSetInteger(ICET_BACKGROUND_COLOR_WORD, 0);
    icetStateSetInteger(ICET_COLOR_FORMAT, ICET_IMAGE_COLOR_RGBA_UBYTE);
    icetStateSetInteger(ICET_DEPTH_FORMAT, ICET_IMAGE_DEPTH_FLOAT);

    icetResetTiles();
    icetStateSetIntegerv(ICET_DISPLAY_NODES, 0, NULL);

    icetStateSetDoublev(ICET_GEOMETRY_BOUNDS, 0, NULL);
    icetStateSetInteger(ICET_NUM_BOUNDING_VERTS, 0);
    icetStateSetPointer(ICET_STRATEGY_COMPOSE, NULL);
    icetCompositeMode(ICET_COMPOSITE_MODE_Z_BUFFER);
    int_array = malloc(ICET_COMM_SIZE() * sizeof(IceTInt));
    for (i = 0; i < ICET_COMM_SIZE(); i++) {
        int_array[i] = i;
    }
    icetStateSetIntegerv(ICET_COMPOSITE_ORDER, ICET_COMM_SIZE(), int_array);
    icetStateSetIntegerv(ICET_PROCESS_ORDERS, ICET_COMM_SIZE(), int_array);
    free(int_array);

    icetStateSetInteger(ICET_DATA_REPLICATION_GROUP, ICET_COMM_RANK());
    icetStateSetInteger(ICET_DATA_REPLICATION_GROUP_SIZE, 1);

    icetStateSetPointer(ICET_DRAW_FUNCTION, NULL);
    icetStateSetInteger(ICET_FRAME_COUNT, 0);

    icetEnable(ICET_FLOATING_VIEWPORT);
    icetDisable(ICET_ORDERED_COMPOSITE);
    icetDisable(ICET_CORRECT_COLORED_BACKGROUND);
    icetEnable(ICET_COMPOSITE_ONE_BUFFER);

    icetStateSetBoolean(ICET_IS_DRAWING_FRAME, 0);
    icetStateSetBoolean(ICET_RENDER_BUFFER_SIZE, 0);

    icetStateResetTiming();
}

void icetStateSetDoublev(IceTEnum pname, IceTInt size, const IceTDouble *data)
{
    stateSet(pname, size, ICET_DOUBLE, data);
}
void icetStateSetFloatv(IceTEnum pname, IceTInt size, const IceTFloat *data)
{
    stateSet(pname, size, ICET_FLOAT, data);
}
void icetStateSetIntegerv(IceTEnum pname, IceTInt size, const IceTInt *data)
{
    stateSet(pname, size, ICET_INT, data);
}
void icetStateSetBooleanv(IceTEnum pname, IceTInt size, const IceTBoolean *data)
{
    stateSet(pname, size, ICET_BOOLEAN, data);
}
void icetStateSetPointerv(IceTEnum pname, IceTInt size, const IceTVoid **data)
{
    stateSet(pname, size, ICET_POINTER, data);
}

void icetStateSetDouble(IceTEnum pname, IceTDouble value)
{
    stateSet(pname, 1, ICET_DOUBLE, &value);
}
void icetStateSetFloat(IceTEnum pname, IceTFloat value)
{
    stateSet(pname, 1, ICET_FLOAT, &value);
}
void icetStateSetInteger(IceTEnum pname, IceTInt value)
{
    stateSet(pname, 1, ICET_INT, &value);
}
void icetStateSetBoolean(IceTEnum pname, IceTBoolean value)
{
    stateSet(pname, 1, ICET_BOOLEAN, &value);
}
void icetStateSetPointer(IceTEnum pname, const IceTVoid *value)
{
    stateSet(pname, 1, ICET_POINTER, &value);
}

IceTEnum icetStateGetType(IceTEnum pname)
{
    return icetGetState()[pname].type;
}
IceTInt icetStateGetSize(IceTEnum pname)
{
    return icetGetState()[pname].size;
}
IceTTimeStamp icetStateGetTime(IceTEnum pname)
{
    return icetGetState()[pname].mod_time;
}

#define copyArrayGivenCType(type_dest, array_dest, type_src, array_src, size) \
    for (i = 0; i < (size); i++) {                                      \
        ((type_dest *)(array_dest))[i]                                  \
            = (type_dest)(((type_src *)(array_src))[i]);                \
    }
#define copyArray(type_dest, array_dest, type_src, array_src, size)            \
    switch (type_src) {                                                        \
      case ICET_DOUBLE:                                                        \
          copyArrayGivenCType(type_dest,array_dest, IceTDouble,array_src, size); \
          break;                                                               \
      case ICET_FLOAT:                                                         \
          copyArrayGivenCType(type_dest,array_dest, IceTFloat,array_src, size);  \
          break;                                                               \
      case ICET_BOOLEAN:                                                       \
          copyArrayGivenCType(type_dest,array_dest, IceTBoolean,array_src, size);\
          break;                                                               \
      case ICET_INT:                                                           \
          copyArrayGivenCType(type_dest,array_dest, IceTInt,array_src, size);    \
          break;                                                               \
      case ICET_NULL:                                                          \
          {                                                                    \
              char msg[256];                                                   \
              sprintf(msg, "No such parameter, 0x%x.", (int)pname);            \
              icetRaiseError(msg, ICET_INVALID_ENUM);                          \
          }                                                                    \
          break;                                                               \
      default:                                                                 \
          {                                                                    \
              char msg[256];                                                   \
              sprintf(msg, "Could not cast value for 0x%x.", (int)pname);      \
              icetRaiseError(msg, ICET_BAD_CAST);                              \
          }                                                                    \
    }

void icetGetDoublev(IceTEnum pname, IceTDouble *params)
{
    struct IceTStateValue *value = icetGetState() + pname;
    int i;
    copyArray(IceTDouble, params, value->type, value->data, value->size);
}
void icetGetFloatv(IceTEnum pname, IceTFloat *params)
{
    struct IceTStateValue *value = icetGetState() + pname;
    int i;
    copyArray(IceTFloat, params, value->type, value->data, value->size);
}
void icetGetBooleanv(IceTEnum pname, IceTBoolean *params)
{
    struct IceTStateValue *value = icetGetState() + pname;
    int i;
    copyArray(IceTBoolean, params, value->type, value->data, value->size);
}
void icetGetIntegerv(IceTEnum pname, IceTInt *params)
{
    struct IceTStateValue *value = icetGetState() + pname;
    int i;
    copyArray(IceTInt, params, value->type, value->data, value->size);
}

void icetGetEnumv(IceTEnum pname, IceTEnum *params)
{
    struct IceTStateValue *value = icetGetState() + pname;
    int i;
    if ((value->type == ICET_FLOAT) || (value->type == ICET_DOUBLE)) {
        icetRaiseError("Floating point values cannot be enumerations.",
                       ICET_BAD_CAST);
    }
    copyArray(IceTEnum, params, value->type, value->data, value->size);
}
void icetGetBitFieldv(IceTEnum pname, IceTBitField *params)
{
    struct IceTStateValue *value = icetGetState() + pname;
    int i;
    if ((value->type == ICET_FLOAT) || (value->type == ICET_DOUBLE)) {
        icetRaiseError("Floating point values cannot be enumerations.",
                       ICET_BAD_CAST);
    }
    copyArray(IceTBitField, params, value->type, value->data, value->size);
}

void icetGetPointerv(IceTEnum pname, IceTVoid **params)
{
    struct IceTStateValue *value = icetGetState() + pname;
    int i;
    if (value->type == ICET_NULL) {
        char msg[256];
        sprintf(msg, "No such parameter, 0x%x.", (int)pname);
        icetRaiseError(msg, ICET_INVALID_ENUM);
    }
    if (value->type != ICET_POINTER) {
        char msg[256];
        sprintf(msg, "Could not cast value for 0x%x.", (int)pname);
        icetRaiseError(msg, ICET_BAD_CAST);
    }
    copyArrayGivenCType(IceTVoid *, params, IceTVoid *, value->data, value->size);
}

void icetEnable(IceTEnum pname)
{
    if ((pname < ICET_STATE_ENABLE_START) || (pname >= ICET_STATE_ENABLE_END)) {
        icetRaiseError("Bad value to icetEnable", ICET_INVALID_VALUE);
        return;
    }
    icetStateSetBoolean(pname, ICET_TRUE);
}

void icetDisable(IceTEnum pname)
{
    if ((pname < ICET_STATE_ENABLE_START) || (pname >= ICET_STATE_ENABLE_END)) {
        icetRaiseError("Bad value to icetDisable", ICET_INVALID_VALUE);
        return;
    }
    icetStateSetBoolean(pname, ICET_FALSE);
}

IceTBoolean icetIsEnabled(IceTEnum pname)
{
    IceTBoolean isEnabled;

    if ((pname < ICET_STATE_ENABLE_START) || (pname >= ICET_STATE_ENABLE_END)) {
        icetRaiseError("Bad value to icetIsEnabled", ICET_INVALID_VALUE);
        return ICET_FALSE;
    }
    icetGetBooleanv(pname, &isEnabled);
    return isEnabled;
}

static int typeWidth(IceTEnum type)
{
    switch (type) {
      case ICET_DOUBLE:
          return sizeof(IceTDouble);
      case ICET_FLOAT:
          return sizeof(IceTFloat);
      case ICET_BOOLEAN:
          return sizeof(IceTBoolean);
      case ICET_SHORT:
          return sizeof(IceTShort);
      case ICET_INT:
          return sizeof(IceTInt);
      case ICET_POINTER:
          return sizeof(IceTVoid *);
      case ICET_NULL:
          return 0;
      default:
          icetRaiseError("Bad type detected in state.", ICET_SANITY_CHECK_FAIL);
    }

    return 0;
}

void icetUnsafeStateSet(IceTEnum pname, IceTInt size, IceTEnum type, IceTVoid *data)
{
    IceTState state = icetGetState();

    if (state[pname].type != ICET_NULL) {
        free(state[pname].data);
    }

    state[pname].type = type;
    state[pname].size = size;
    state[pname].mod_time = icetGetTimeStamp();
    state[pname].data = data;
}

static void stateSet(IceTEnum pname, IceTInt size, IceTEnum type, const IceTVoid *data)
{
    IceTState state;
    int type_width;
    void *datacopy;

    state = icetGetState();
    type_width = typeWidth(type);

    if ((size == state[pname].size) && (type == state[pname].type)) {
      /* Save time by just copying data into pre-existing memory. */
        memcpy(state[pname].data, data, size * type_width);
        state[pname].mod_time = icetGetTimeStamp();
    } else {
        datacopy = malloc(size * type_width);
        memcpy(datacopy, data, size * type_width);

        icetUnsafeStateSet(pname, size, type, datacopy);
    }
}

static void *icetUnsafeStateGet(IceTEnum pname, IceTEnum type)
{
    if (icetGetState()[pname].type != type) {
	icetRaiseError("Mismatched types in unsafe state get.",
		       ICET_SANITY_CHECK_FAIL);
	return NULL;
    }
    return icetGetState()[pname].data;
}

IceTDouble *icetUnsafeStateGetDouble(IceTEnum pname)
{
    return icetUnsafeStateGet(pname, ICET_DOUBLE);
}
IceTFloat *icetUnsafeStateGetFloat(IceTEnum pname)
{
    return icetUnsafeStateGet(pname, ICET_FLOAT);
}
IceTInt *icetUnsafeStateGetInteger(IceTEnum pname)
{
    return icetUnsafeStateGet(pname, ICET_INT);
}
IceTBoolean *icetUnsafeStateGetBoolean(IceTEnum pname)
{
    return icetUnsafeStateGet(pname, ICET_BOOLEAN);
}
IceTVoid **icetUnsafeStateGetPointer(IceTEnum pname)
{
    return icetUnsafeStateGet(pname, ICET_POINTER);
}

IceTEnum icetStateType(IceTEnum pname)
{
    return icetGetState()[pname].type;
}

IceTTimeStamp icetGetTimeStamp(void)
{
    static IceTTimeStamp current_time = 0;

    return current_time++;
}

void icetStateResetTiming(void)
{
    icetStateSetDouble(ICET_RENDER_TIME, 0.0);
    icetStateSetDouble(ICET_BUFFER_READ_TIME, 0.0);
    icetStateSetDouble(ICET_BUFFER_WRITE_TIME, 0.0);
    icetStateSetDouble(ICET_COMPRESS_TIME, 0.0);
    icetStateSetDouble(ICET_COMPARE_TIME, 0.0);
    icetStateSetDouble(ICET_COMPOSITE_TIME, 0.0);
    icetStateSetDouble(ICET_TOTAL_DRAW_TIME, 0.0);

    icetStateSetInteger(ICET_BYTES_SENT, 0);
}

void icetStateDump(void)
{
    IceTEnum i;
    IceTState state;

    state = icetGetState();
    printf("State dump:\n");
    for (i = 0; i < ICET_STATE_SIZE; i++) {
        if (state->type != ICET_NULL) {
            printf("param = 0x%x\n", i);
            printf("type  = 0x%x\n", (int)state->type);
            printf("size  = %d\n", (int)state->size);
            printf("data  = %p\n", state->data);
            printf("mod   = %d\n", (int)state->mod_time);
        }
        state++;
    }
}
