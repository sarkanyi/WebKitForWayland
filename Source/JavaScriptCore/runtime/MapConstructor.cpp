/*
 * Copyright (C) 2013, 2016 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MapConstructor.h"

#include "Error.h"
#include "GetterSetter.h"
#include "IteratorOperations.h"
#include "JSCJSValueInlines.h"
#include "JSCellInlines.h"
#include "JSGlobalObject.h"
#include "JSMap.h"
#include "MapPrototype.h"
#include "StructureInlines.h"

namespace JSC {

const ClassInfo MapConstructor::s_info = { "Function", &Base::s_info, 0, CREATE_METHOD_TABLE(MapConstructor) };

void MapConstructor::finishCreation(VM& vm, MapPrototype* mapPrototype, GetterSetter* speciesSymbol)
{
    Base::finishCreation(vm, mapPrototype->classInfo()->className);
    putDirectWithoutTransition(vm, vm.propertyNames->prototype, mapPrototype, DontEnum | DontDelete | ReadOnly);
    putDirectWithoutTransition(vm, vm.propertyNames->length, jsNumber(0), ReadOnly | DontEnum | DontDelete);
    putDirectNonIndexAccessor(vm, vm.propertyNames->speciesSymbol, speciesSymbol, Accessor | ReadOnly | DontEnum);
}

static EncodedJSValue JSC_HOST_CALL callMap(ExecState* exec)
{
    return JSValue::encode(throwConstructorCannotBeCalledAsFunctionTypeError(exec, "Map"));
}

static EncodedJSValue JSC_HOST_CALL constructMap(ExecState* exec)
{
    JSGlobalObject* globalObject = asInternalFunction(exec->callee())->globalObject();
    Structure* mapStructure = InternalFunction::createSubclassStructure(exec, exec->newTarget(), globalObject->mapStructure());
    if (exec->hadException())
        return JSValue::encode(JSValue());
    JSMap* map = JSMap::create(exec, mapStructure);
    JSValue iterable = exec->argument(0);
    if (iterable.isUndefinedOrNull())
        return JSValue::encode(map);

    JSValue adderFunction = map->JSObject::get(exec, exec->propertyNames().set);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    CallData adderFunctionCallData;
    CallType adderFunctionCallType = getCallData(adderFunction, adderFunctionCallData);
    if (adderFunctionCallType == CallType::None)
        return JSValue::encode(throwTypeError(exec));

    forEachInIterable(exec, iterable, [&](VM& vm, ExecState* exec, JSValue nextItem) {
        if (!nextItem.isObject()) {
            throwTypeError(exec);
            return;
        }

        JSValue key = nextItem.get(exec, static_cast<unsigned>(0));
        if (vm.exception())
            return;

        JSValue value = nextItem.get(exec, static_cast<unsigned>(1));
        if (vm.exception())
            return;

        MarkedArgumentBuffer arguments;
        arguments.append(key);
        arguments.append(value);
        call(exec, adderFunction, adderFunctionCallType, adderFunctionCallData, map, arguments);
    });

    return JSValue::encode(map);
}

ConstructType MapConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructMap;
    return ConstructType::Host;
}

CallType MapConstructor::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callMap;
    return CallType::Host;
}

}
