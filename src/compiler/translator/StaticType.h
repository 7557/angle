//
// Copyright (c) 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Compile-time instances of many common TType values. These are looked up
// (statically or dynamically) through the methods defined in the namespace.
//

#ifndef COMPILER_TRANSLATOR_STATIC_TYPE_H_
#define COMPILER_TRANSLATOR_STATIC_TYPE_H_

#include "compiler/translator/Types.h"

namespace sh
{

namespace StaticType
{

namespace Helpers
{

//
// Generation and static allocation of type mangled name values.
//

// Size of the maximum possible constexpr-generated mangled name.
// If this value is too small, the compiler will produce errors.
static constexpr size_t kStaticMangledNameMaxLength = 10;

// Type which holds the mangled names for constexpr-generated TTypes.
// This simple struct is needed so that a char array can be returned by value.
struct StaticMangledName
{
    // If this array is too small, the compiler will produce errors.
    char name[kStaticMangledNameMaxLength + 1] = {};
};

// Generates a mangled name for a TType given its parameters.
constexpr StaticMangledName BuildStaticMangledName(TBasicType basicType,
                                                   TPrecision precision,
                                                   TQualifier qualifier,
                                                   unsigned char primarySize,
                                                   unsigned char secondarySize)
{
    StaticMangledName name = {};
    // When this function is executed constexpr (should be always),
    // name.name[at] is guaranteed by the compiler to never go out of bounds.
    size_t at = 0;

    bool isMatrix = primarySize > 1 && secondarySize > 1;
    bool isVector = primarySize > 1 && secondarySize == 1;

    if (isMatrix)
    {
        name.name[at++] = 'm';
    }
    else if (isVector)
    {
        name.name[at++] = 'v';
    }

    {
        const char *basicMangledName = GetBasicMangledName(basicType);
        for (size_t i = 0; basicMangledName[i] != '\0'; ++i)
        {
            name.name[at++] = basicMangledName[i];
        }
    }

    name.name[at++] = '0' + primarySize;
    if (isMatrix)
    {
        name.name[at++] = 'x';
        name.name[at++] = '0' + secondarySize;
    }

    name.name[at++] = ';';

    name.name[at] = '\0';
    return name;
}

// This "variable" contains the mangled names for every constexpr-generated TType.
// If kMangledNameInstance<B, P, Q, PS, SS> is used anywhere (specifally
// in kInstance, below), this is where the appropriate type will be stored.
template <TBasicType basicType,
          TPrecision precision,
          TQualifier qualifier,
          unsigned char primarySize,
          unsigned char secondarySize>
static constexpr StaticMangledName kMangledNameInstance =
    BuildStaticMangledName(basicType, precision, qualifier, primarySize, secondarySize);

//
// Generation and static allocation of TType values.
//

// This "variable" contains every constexpr-generated TType.
// If kInstance<B, P, Q, PS, SS> is used anywhere (specifally
// in Get, below), this is where the appropriate type will be stored.
template <TBasicType basicType,
          TPrecision precision,
          TQualifier qualifier,
          unsigned char primarySize,
          unsigned char secondarySize>
static constexpr TType kInstance =
    TType(basicType,
          precision,
          qualifier,
          primarySize,
          secondarySize,
          kMangledNameInstance<basicType, precision, qualifier, primarySize, secondarySize>.name);

}  // namespace Helpers

//
// Fully-qualified type lookup.
//

template <TBasicType basicType,
          TPrecision precision,
          TQualifier qualifier,
          unsigned char primarySize,
          unsigned char secondarySize>
constexpr const TType *Get()
{
    static_assert(1 <= primarySize && primarySize <= 4, "primarySize out of bounds");
    static_assert(1 <= secondarySize && secondarySize <= 4, "secondarySize out of bounds");
    return &Helpers::kInstance<basicType, precision, qualifier, primarySize, secondarySize>;
}

//
// Overloads
//

template <TBasicType basicType, unsigned char primarySize = 1, unsigned char secondarySize = 1>
constexpr const TType *GetBasic()
{
    return Get<basicType, EbpUndefined, EvqGlobal, primarySize, secondarySize>();
}

template <TBasicType basicType,
          TQualifier qualifier,
          unsigned char primarySize   = 1,
          unsigned char secondarySize = 1>
const TType *GetQualified()
{
    return Get<basicType, EbpUndefined, qualifier, primarySize, secondarySize>();
}

// Dynamic lookup methods (convert runtime values to template args)

namespace Helpers
{

// Helper which takes secondarySize statically but primarySize dynamically.
template <TBasicType basicType,
          TPrecision precision,
          TQualifier qualifier,
          unsigned char secondarySize>
constexpr const TType *GetForVecMatHelper(unsigned char primarySize)
{
    static_assert(basicType == EbtFloat || basicType == EbtInt || basicType == EbtUInt ||
                      basicType == EbtBool,
                  "unsupported basicType");
    switch (primarySize)
    {
        case 1:
            return Get<basicType, precision, qualifier, 1, secondarySize>();
        case 2:
            return Get<basicType, precision, qualifier, 2, secondarySize>();
        case 3:
            return Get<basicType, precision, qualifier, 3, secondarySize>();
        case 4:
            return Get<basicType, precision, qualifier, 4, secondarySize>();
        default:
            UNREACHABLE();
            return GetBasic<EbtVoid>();
    }
}

}  // namespace Helpers

template <TBasicType basicType,
          TPrecision precision = EbpUndefined,
          TQualifier qualifier = EvqGlobal>
constexpr const TType *GetForVecMat(unsigned char primarySize, unsigned char secondarySize = 1)
{
    static_assert(basicType == EbtFloat || basicType == EbtInt || basicType == EbtUInt ||
                      basicType == EbtBool,
                  "unsupported basicType");
    switch (secondarySize)
    {
        case 1:
            return Helpers::GetForVecMatHelper<basicType, precision, qualifier, 1>(primarySize);
        case 2:
            return Helpers::GetForVecMatHelper<basicType, precision, qualifier, 2>(primarySize);
        case 3:
            return Helpers::GetForVecMatHelper<basicType, precision, qualifier, 3>(primarySize);
        case 4:
            return Helpers::GetForVecMatHelper<basicType, precision, qualifier, 4>(primarySize);
        default:
            UNREACHABLE();
            return GetBasic<EbtVoid>();
    }
}

template <TBasicType basicType, TPrecision precision = EbpUndefined>
constexpr const TType *GetForVec(TQualifier qualifier, unsigned char size)
{
    switch (qualifier)
    {
        case EvqGlobal:
            return Helpers::GetForVecMatHelper<basicType, precision, EvqGlobal, 1>(size);
        case EvqOut:
            return Helpers::GetForVecMatHelper<basicType, precision, EvqOut, 1>(size);
        default:
            UNREACHABLE();
            return GetBasic<EbtVoid>();
    }
}

const TType *GetForFloatImage(TBasicType basicType);

const TType *GetForIntImage(TBasicType basicType);

const TType *GetForUintImage(TBasicType basicType);

}  // namespace StaticType

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_STATIC_TYPE_H_
