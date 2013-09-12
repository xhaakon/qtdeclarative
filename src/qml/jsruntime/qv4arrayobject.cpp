/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4arrayobject_p.h"
#include "qv4sparsearray_p.h"
#include "qv4objectproto_p.h"
#include "qv4scopedvalue_p.h"

using namespace QV4;

DEFINE_MANAGED_VTABLE(ArrayCtor);

ArrayCtor::ArrayCtor(ExecutionContext *scope)
    : FunctionObject(scope, scope->engine->newIdentifier(QStringLiteral("Array")))
{
    vtbl = &static_vtbl;
}

ReturnedValue ArrayCtor::construct(Managed *m, CallData *callData)
{
    ExecutionEngine *v4 = m->engine();
    ArrayObject *a = v4->newArrayObject();
    uint len;
    if (callData->argc == 1 && callData->args[0].isNumber()) {
        bool ok;
        len = callData->args[0].asArrayLength(&ok);

        if (!ok)
            v4->current->throwRangeError(callData->args[0]);

        if (len < 0x1000)
            a->arrayReserve(len);
    } else {
        len = callData->argc;
        a->arrayReserve(len);
        for (unsigned int i = 0; i < len; ++i)
            a->arrayData[i].value = callData->args[i];
        a->arrayDataLen = len;
    }
    a->setArrayLengthUnchecked(len);

    return Value::fromObject(a).asReturnedValue();
}

ReturnedValue ArrayCtor::call(Managed *that, CallData *callData)
{
    return construct(that, callData);
}

ArrayPrototype::ArrayPrototype(InternalClass *ic)
    : ArrayObject(ic)
{
}

void ArrayPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("isArray"), method_isArray, 1);
    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString, 0);
    defineDefaultProperty(ctx, QStringLiteral("toLocaleString"), method_toLocaleString, 0);
    defineDefaultProperty(ctx, QStringLiteral("concat"), method_concat, 1);
    defineDefaultProperty(ctx, QStringLiteral("join"), method_join, 1);
    defineDefaultProperty(ctx, QStringLiteral("pop"), method_pop, 0);
    defineDefaultProperty(ctx, QStringLiteral("push"), method_push, 1);
    defineDefaultProperty(ctx, QStringLiteral("reverse"), method_reverse, 0);
    defineDefaultProperty(ctx, QStringLiteral("shift"), method_shift, 0);
    defineDefaultProperty(ctx, QStringLiteral("slice"), method_slice, 2);
    defineDefaultProperty(ctx, QStringLiteral("sort"), method_sort, 1);
    defineDefaultProperty(ctx, QStringLiteral("splice"), method_splice, 2);
    defineDefaultProperty(ctx, QStringLiteral("unshift"), method_unshift, 1);
    defineDefaultProperty(ctx, QStringLiteral("indexOf"), method_indexOf, 1);
    defineDefaultProperty(ctx, QStringLiteral("lastIndexOf"), method_lastIndexOf, 1);
    defineDefaultProperty(ctx, QStringLiteral("every"), method_every, 1);
    defineDefaultProperty(ctx, QStringLiteral("some"), method_some, 1);
    defineDefaultProperty(ctx, QStringLiteral("forEach"), method_forEach, 1);
    defineDefaultProperty(ctx, QStringLiteral("map"), method_map, 1);
    defineDefaultProperty(ctx, QStringLiteral("filter"), method_filter, 1);
    defineDefaultProperty(ctx, QStringLiteral("reduce"), method_reduce, 1);
    defineDefaultProperty(ctx, QStringLiteral("reduceRight"), method_reduceRight, 1);
}

uint ArrayPrototype::getLength(ExecutionContext *ctx, Object *o)
{
    if (o->isArrayObject())
        return o->arrayLength();
    Scope scope(ctx);
    ScopedValue v(scope, o->get(ctx->engine->id_length));
    return v->toUInt32();
}

ReturnedValue ArrayPrototype::method_isArray(SimpleCallContext *ctx)
{
    Value arg = ctx->argument(0);
    bool isArray = arg.asArrayObject();
    return Value::fromBoolean(isArray).asReturnedValue();
}

ReturnedValue ArrayPrototype::method_toString(SimpleCallContext *ctx)
{
    QV4::Scope scope(ctx);
    QV4::Object *o = ctx->thisObject.toObject(ctx);
    ScopedValue v(scope, o->get(ctx->engine->newString("join")));
    FunctionObject *f = v->asFunctionObject();
    if (f) {
        ScopedCallData d(scope, 0);
        d->thisObject = ctx->thisObject;
        return f->call(d);
    }
    return ObjectPrototype::method_toString(ctx);
}

ReturnedValue ArrayPrototype::method_toLocaleString(SimpleCallContext *ctx)
{
    return method_toString(ctx);
}

ReturnedValue ArrayPrototype::method_concat(SimpleCallContext *ctx)
{
    ArrayObject *result = ctx->engine->newArrayObject();

    if (ArrayObject *instance = ctx->thisObject.asArrayObject()) {
        result->copyArrayData(instance);
    } else if (ctx->thisObject.isString()) {
        QString v = ctx->thisObject.stringValue()->toQString();
        result->arraySet(0, Value::fromString(ctx, v));
    } else if (ctx->thisObject.asStringObject()) {
        QString v = ctx->thisObject.toString(ctx)->toQString();
        result->arraySet(0, Value::fromString(ctx, v));
    } else {
        Object *instance = ctx->thisObject.toObject(ctx);
        result->arraySet(0, Value::fromObject(instance));
    }

    for (uint i = 0; i < ctx->argumentCount; ++i) {
        Value arg = ctx->argument(i);

        if (ArrayObject *elt = arg.asArrayObject())
            result->arrayConcat(elt);

        else
            result->arraySet(getLength(ctx, result), arg);
    }

    return Value::fromObject(result).asReturnedValue();
}

ReturnedValue ArrayPrototype::method_join(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Value arg = ctx->argument(0);

    QString r4;
    if (arg.isUndefined())
        r4 = QStringLiteral(",");
    else
        r4 = arg.toString(ctx)->toQString();

    Scoped<Object> self(scope, ctx->thisObject);
    ScopedValue length(scope, self->get(ctx->engine->id_length));
    const quint32 r2 = Value::toUInt32(length->isUndefined() ? 0 : length->toNumber());

    static QSet<Object *> visitedArrayElements;

    if (! r2 || visitedArrayElements.contains(self.getPointer()))
        return Value::fromString(ctx, QString()).asReturnedValue();

    // avoid infinite recursion
    visitedArrayElements.insert(self.getPointer());

    QString R;

    // ### FIXME
    if (ArrayObject *a = self->asArrayObject()) {
        ScopedValue e(scope);
        for (uint i = 0; i < a->arrayLength(); ++i) {
            if (i)
                R += r4;

            e = a->getIndexed(i);
            if (!e->isNullOrUndefined())
                R += e->toString(ctx)->toQString();
        }
    } else {
        //
        // crazy!
        //
        ScopedValue r6(scope, self->get(ctx->engine->newString(QStringLiteral("0"))));
        if (!r6->isNullOrUndefined())
            R = r6->toString(ctx)->toQString();

        ScopedValue r12(scope);
        for (quint32 k = 1; k < r2; ++k) {
            R += r4;

            String *name = Value::fromDouble(k).toString(ctx);
            r12 = self->get(name);

            if (!r12->isNullOrUndefined())
                R += r12->toString(ctx)->toQString();
        }
    }

    visitedArrayElements.remove(self.getPointer());
    return Value::fromString(ctx, R).asReturnedValue();
}

ReturnedValue ArrayPrototype::method_pop(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Object *instance = ctx->thisObject.toObject(ctx);
    uint len = getLength(ctx, instance);

    if (!len) {
        if (!instance->isArrayObject())
            instance->put(ctx->engine->id_length, Value::fromInt32(0));
        return Value::undefinedValue().asReturnedValue();
    }

    ScopedValue result(scope, instance->getIndexed(len - 1));

    instance->deleteIndexedProperty(len - 1);
    if (instance->isArrayObject())
        instance->setArrayLengthUnchecked(len - 1);
    else
        instance->put(ctx->engine->id_length, Value::fromDouble(len - 1));
    return result.asReturnedValue();
}

ReturnedValue ArrayPrototype::method_push(SimpleCallContext *ctx)
{
    Object *instance = ctx->thisObject.toObject(ctx);
    uint len = getLength(ctx, instance);

    if (len + ctx->argumentCount < len) {
        // ughh...
        double l = len;
        for (double i = 0; i < ctx->argumentCount; ++i) {
            Value idx = Value::fromDouble(l + i);
            instance->put(idx.toString(ctx), ctx->argument(i));
        }
        double newLen = l + ctx->argumentCount;
        if (!instance->isArrayObject())
            instance->put(ctx->engine->id_length, Value::fromDouble(newLen));
        else
            ctx->throwRangeError(Value::fromString(ctx, QStringLiteral("Array.prototype.push: Overflow")));
        return Value::fromDouble(newLen).asReturnedValue();
    }

    if (!instance->protoHasArray() && instance->arrayDataLen <= len) {
        for (uint i = 0; i < ctx->argumentCount; ++i) {
            Value v = ctx->argument(i);

            if (!instance->sparseArray) {
                if (len >= instance->arrayAlloc)
                    instance->arrayReserve(len + 1);
                instance->arrayData[len].value = v;
                if (instance->arrayAttributes)
                    instance->arrayAttributes[len] = Attr_Data;
                instance->arrayDataLen = len + 1;
            } else {
                uint i = instance->allocArrayValue(v);
                instance->sparseArray->push_back(i, len);
            }
            ++len;
        }
    } else {
        for (uint i = 0; i < ctx->argumentCount; ++i)
            instance->putIndexed(len + i, ctx->argument(i));
        len += ctx->argumentCount;
    }
    if (instance->isArrayObject())
        instance->setArrayLengthUnchecked(len);
    else
        instance->put(ctx->engine->id_length, Value::fromDouble(len));

    if (len < INT_MAX)
        return Value::fromInt32(len).asReturnedValue();
    return Value::fromDouble((double)len).asReturnedValue();

}

ReturnedValue ArrayPrototype::method_reverse(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Object *instance = ctx->thisObject.toObject(ctx);
    uint length = getLength(ctx, instance);

    int lo = 0, hi = length - 1;

    ScopedValue lval(scope);
    ScopedValue hval(scope);
    for (; lo < hi; ++lo, --hi) {
        bool loExists, hiExists;
        lval = instance->getIndexed(lo, &loExists);
        hval = instance->getIndexed(hi, &hiExists);
        if (hiExists)
            instance->putIndexed(lo, hval);
        else
            instance->deleteIndexedProperty(lo);
        if (loExists)
            instance->putIndexed(hi, lval);
        else
            instance->deleteIndexedProperty(hi);
    }
    return Value::fromObject(instance).asReturnedValue();
}

ReturnedValue ArrayPrototype::method_shift(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Object *instance = ctx->thisObject.toObject(ctx);
    uint len = getLength(ctx, instance);

    if (!len) {
        if (!instance->isArrayObject())
            instance->put(ctx->engine->id_length, Value::fromInt32(0));
        return Value::undefinedValue().asReturnedValue();
    }

    Property *front = 0;
    uint pidx = instance->propertyIndexFromArrayIndex(0);
    if (pidx < UINT_MAX && (!instance->arrayAttributes || !instance->arrayAttributes[0].isGeneric()))
            front = instance->arrayData + pidx;

    Value result = front ? Value::fromReturnedValue(instance->getValue(front, instance->arrayAttributes ? instance->arrayAttributes[pidx] : Attr_Data)) : Value::undefinedValue();

    if (!instance->protoHasArray() && instance->arrayDataLen <= len) {
        if (!instance->sparseArray) {
            if (instance->arrayDataLen) {
                ++instance->arrayOffset;
                ++instance->arrayData;
                --instance->arrayDataLen;
                --instance->arrayAlloc;
                if (instance->arrayAttributes)
                    ++instance->arrayAttributes;
            }
        } else {
            uint idx = instance->sparseArray->pop_front();
            instance->freeArrayValue(idx);
        }
    } else {
        ScopedValue v(scope);
        // do it the slow way
        for (uint k = 1; k < len; ++k) {
            bool exists;
            v = instance->getIndexed(k, &exists);
            if (exists)
                instance->putIndexed(k - 1, v);
            else
                instance->deleteIndexedProperty(k - 1);
        }
        instance->deleteIndexedProperty(len - 1);
    }

    if (instance->isArrayObject())
        instance->setArrayLengthUnchecked(len - 1);
    else
        instance->put(ctx->engine->id_length, Value::fromDouble(len - 1));
    return result.asReturnedValue();
}

ReturnedValue ArrayPrototype::method_slice(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Object *o = ctx->thisObject.toObject(ctx);

    ArrayObject *result = ctx->engine->newArrayObject();
    uint len = ArrayPrototype::getLength(ctx, o);
    double s = ctx->argument(0).toInteger();
    uint start;
    if (s < 0)
        start = (uint)qMax(len + s, 0.);
    else if (s > len)
        start = len;
    else
        start = (uint) s;
    uint end = len;
    if (!ctx->argument(1).isUndefined()) {
        double e = ctx->argument(1).toInteger();
        if (e < 0)
            end = (uint)qMax(len + e, 0.);
        else if (e > len)
            end = len;
        else
            end = (uint) e;
    }

    ScopedValue v(scope);
    uint n = 0;
    for (uint i = start; i < end; ++i) {
        bool exists;
        v = o->getIndexed(i, &exists);
        if (exists) {
            result->arraySet(n, v);
        }
        ++n;
    }
    return Value::fromObject(result).asReturnedValue();
}

ReturnedValue ArrayPrototype::method_sort(SimpleCallContext *ctx)
{
    Object *instance = ctx->thisObject.toObject(ctx);

    uint len = getLength(ctx, instance);

    Value comparefn = ctx->argument(0);
    instance->arraySort(ctx, instance, comparefn, len);
    return ctx->thisObject.asReturnedValue();
}

ReturnedValue ArrayPrototype::method_splice(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Object *instance = ctx->thisObject.toObject(ctx);
    uint len = getLength(ctx, instance);

    ArrayObject *newArray = ctx->engine->newArrayObject();

    double rs = ctx->argument(0).toInteger();
    uint start;
    if (rs < 0)
        start = (uint) qMax(0., len + rs);
    else
        start = (uint) qMin(rs, (double)len);

    uint deleteCount = (uint)qMin(qMax(ctx->argument(1).toInteger(), 0.), (double)(len - start));

    newArray->arrayReserve(deleteCount);
    Property *pd = newArray->arrayData;
    for (uint i = 0; i < deleteCount; ++i) {
        pd->value = Value::fromReturnedValue(instance->getIndexed(start + i));
        ++pd;
    }
    newArray->arrayDataLen = deleteCount;
    newArray->setArrayLengthUnchecked(deleteCount);

    uint itemCount = ctx->argumentCount < 2 ? 0 : ctx->argumentCount - 2;

    ScopedValue v(scope);
    if (itemCount < deleteCount) {
        for (uint k = start; k < len - deleteCount; ++k) {
            bool exists;
            v = instance->getIndexed(k + deleteCount, &exists);
            if (exists)
                instance->putIndexed(k + itemCount, v);
            else
                instance->deleteIndexedProperty(k + itemCount);
        }
        for (uint k = len; k > len - deleteCount + itemCount; --k)
            instance->deleteIndexedProperty(k - 1);
    } else if (itemCount > deleteCount) {
        uint k = len - deleteCount;
        while (k > start) {
            bool exists;
            v = instance->getIndexed(k + deleteCount - 1, &exists);
            if (exists)
                instance->putIndexed(k + itemCount - 1, v);
            else
                instance->deleteIndexedProperty(k + itemCount - 1);
            --k;
        }
    }

    for (uint i = 0; i < itemCount; ++i)
        instance->putIndexed(start + i, ctx->argument(i + 2));

    ctx->strictMode = true;
    instance->put(ctx->engine->id_length, Value::fromDouble(len - deleteCount + itemCount));

    return Value::fromObject(newArray).asReturnedValue();
}

ReturnedValue ArrayPrototype::method_unshift(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Object *instance = ctx->thisObject.toObject(ctx);
    uint len = getLength(ctx, instance);

    if (!instance->protoHasArray() && instance->arrayDataLen <= len) {
        for (int i = ctx->argumentCount - 1; i >= 0; --i) {
            Value v = ctx->argument(i);

            if (!instance->sparseArray) {
                if (!instance->arrayOffset)
                    instance->getArrayHeadRoom();

                --instance->arrayOffset;
                --instance->arrayData;
                ++instance->arrayDataLen;
                if (instance->arrayAttributes) {
                    --instance->arrayAttributes;
                    *instance->arrayAttributes = Attr_Data;
                }
                instance->arrayData->value = v;
            } else {
                uint idx = instance->allocArrayValue(v);
                instance->sparseArray->push_front(idx);
            }
        }
    } else {
        ScopedValue v(scope);
        for (uint k = len; k > 0; --k) {
            bool exists;
            v = instance->getIndexed(k - 1, &exists);
            if (exists)
                instance->putIndexed(k + ctx->argumentCount - 1, v);
            else
                instance->deleteIndexedProperty(k + ctx->argumentCount - 1);
        }
        for (uint i = 0; i < ctx->argumentCount; ++i)
            instance->putIndexed(i, ctx->argument(i));
    }

    uint newLen = len + ctx->argumentCount;
    if (instance->isArrayObject())
        instance->setArrayLengthUnchecked(newLen);
    else
        instance->put(ctx->engine->id_length, Value::fromDouble(newLen));

    if (newLen < INT_MAX)
        return Value::fromInt32(newLen).asReturnedValue();
    return Value::fromDouble((double)newLen).asReturnedValue();
}

ReturnedValue ArrayPrototype::method_indexOf(SimpleCallContext *ctx)
{
    Scope scope(ctx);

    Object *instance = ctx->thisObject.toObject(ctx);
    uint len = getLength(ctx, instance);
    if (!len)
        return Value::fromInt32(-1).asReturnedValue();

    ScopedValue searchValue(scope);
    uint fromIndex = 0;

    if (ctx->argumentCount >= 1)
        searchValue = ctx->argument(0);
    else
        searchValue = Value::undefinedValue();

    if (ctx->argumentCount >= 2) {
        double f = ctx->argument(1).toInteger();
        if (f >= len)
            return Value::fromInt32(-1).asReturnedValue();
        if (f < 0)
            f = qMax(len + f, 0.);
        fromIndex = (uint) f;
    }

    if (instance->isStringObject()) {
        ScopedValue v(scope);
        for (uint k = fromIndex; k < len; ++k) {
            bool exists;
            v = instance->getIndexed(k, &exists);
            if (exists && __qmljs_strict_equal(v, searchValue))
                return Value::fromDouble(k).asReturnedValue();
        }
        return Value::fromInt32(-1).asReturnedValue();
    }

    return instance->arrayIndexOf(searchValue, fromIndex, len, ctx, instance).asReturnedValue();
}

ReturnedValue ArrayPrototype::method_lastIndexOf(SimpleCallContext *ctx)
{
    Scope scope(ctx);

    Object *instance = ctx->thisObject.toObject(ctx);
    uint len = getLength(ctx, instance);
    if (!len)
        return Value::fromInt32(-1).asReturnedValue();

    ScopedValue searchValue(scope);
    uint fromIndex = len;

    if (ctx->argumentCount >= 1)
        searchValue = ctx->argument(0);
    else
        searchValue = Value::undefinedValue();

    if (ctx->argumentCount >= 2) {
        double f = ctx->argument(1).toInteger();
        if (f > 0)
            f = qMin(f, (double)(len - 1));
        else if (f < 0) {
            f = len + f;
            if (f < 0)
                return Value::fromInt32(-1).asReturnedValue();
        }
        fromIndex = (uint) f + 1;
    }

    ScopedValue v(scope);
    for (uint k = fromIndex; k > 0;) {
        --k;
        bool exists;
        v = instance->getIndexed(k, &exists);
        if (exists && __qmljs_strict_equal(v, searchValue))
            return Value::fromDouble(k).asReturnedValue();
    }
    return Value::fromInt32(-1).asReturnedValue();
}

ReturnedValue ArrayPrototype::method_every(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Object *instance = ctx->thisObject.toObject(ctx);

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        ctx->throwTypeError();

    Value thisArg = ctx->argument(1);

    ScopedCallData callData(scope, 3);
    callData->args[2] = Value::fromObject(instance);
    callData->thisObject = thisArg;
    ScopedValue r(scope);
    ScopedValue v(scope);

    bool ok = true;
    for (uint k = 0; ok && k < len; ++k) {
        bool exists;
        v = instance->getIndexed(k, &exists);
        if (!exists)
            continue;

        callData->args[0] = v;
        callData->args[1] = Value::fromDouble(k);
        r = callback->call(callData);
        ok = r->toBoolean();
    }
    return Value::fromBoolean(ok).asReturnedValue();
}

ReturnedValue ArrayPrototype::method_some(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Object *instance = ctx->thisObject.toObject(ctx);

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        ctx->throwTypeError();

    ScopedCallData callData(scope, 3);
    callData->thisObject = ctx->argument(1);
    callData->args[2] = Value::fromObject(instance);
    ScopedValue v(scope);

    for (uint k = 0; k < len; ++k) {
        bool exists;
        v = instance->getIndexed(k, &exists);
        if (!exists)
            continue;

        callData->args[0] = v;
        callData->args[1] = Value::fromDouble(k);
        Value r = Value::fromReturnedValue(callback->call(callData));
        if (r.toBoolean())
            return Value::fromBoolean(true).asReturnedValue();
    }
    return Value::fromBoolean(false).asReturnedValue();
}

ReturnedValue ArrayPrototype::method_forEach(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Object *instance = ctx->thisObject.toObject(ctx);

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        ctx->throwTypeError();

    ScopedCallData callData(scope, 3);
    callData->thisObject = ctx->argument(1);
    callData->args[2] = Value::fromObject(instance);

    ScopedValue v(scope);
    for (uint k = 0; k < len; ++k) {
        bool exists;
        v = instance->getIndexed(k, &exists);
        if (!exists)
            continue;

        callData->args[0] = v;
        callData->args[1] = Value::fromDouble(k);
        callback->call(callData);
    }
    return Value::undefinedValue().asReturnedValue();
}

ReturnedValue ArrayPrototype::method_map(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Object *instance = ctx->thisObject.toObject(ctx);

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        ctx->throwTypeError();

    Value thisArg = ctx->argument(1);

    ArrayObject *a = ctx->engine->newArrayObject();
    a->arrayReserve(len);
    a->setArrayLengthUnchecked(len);

    ScopedValue mapped(scope);
    ScopedCallData callData(scope, 3);
    callData->thisObject = thisArg;
    callData->args[2] = Value::fromObject(instance);

    ScopedValue v(scope);
    for (uint k = 0; k < len; ++k) {
        bool exists;
        v = instance->getIndexed(k, &exists);
        if (!exists)
            continue;

        callData->args[0] = v;
        callData->args[1] = Value::fromDouble(k);
        mapped = callback->call(callData);
        a->arraySet(k, mapped);
    }
    return Value::fromObject(a).asReturnedValue();
}

ReturnedValue ArrayPrototype::method_filter(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Object *instance = ctx->thisObject.toObject(ctx);

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        ctx->throwTypeError();

    Value thisArg = ctx->argument(1);

    ArrayObject *a = ctx->engine->newArrayObject();
    a->arrayReserve(len);

    ScopedValue selected(scope);
    ScopedCallData callData(scope, 3);
    callData->thisObject = thisArg;
    callData->args[2] = Value::fromObject(instance);

    ScopedValue v(scope);

    uint to = 0;
    for (uint k = 0; k < len; ++k) {
        bool exists;
        v = instance->getIndexed(k, &exists);
        if (!exists)
            continue;

        callData->args[0] = v;
        callData->args[1] = Value::fromDouble(k);
        selected = callback->call(callData);
        if (selected->toBoolean()) {
            a->arraySet(to, v);
            ++to;
        }
    }
    return Value::fromObject(a).asReturnedValue();
}

ReturnedValue ArrayPrototype::method_reduce(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Object *instance = ctx->thisObject.toObject(ctx);

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        ctx->throwTypeError();

    uint k = 0;
    ScopedValue acc(scope);
    ScopedValue v(scope);

    if (ctx->argumentCount > 1) {
        acc = ctx->argument(1);
    } else {
        bool kPresent = false;
        while (k < len && !kPresent) {
            v = instance->getIndexed(k, &kPresent);
            if (kPresent)
                acc = v;
            ++k;
        }
        if (!kPresent)
            ctx->throwTypeError();
    }

    ScopedCallData callData(scope, 4);
    callData->thisObject = Value::undefinedValue();
    callData->args[0] = acc;
    callData->args[3] = Value::fromObject(instance);

    while (k < len) {
        bool kPresent;
        v = instance->getIndexed(k, &kPresent);
        if (kPresent) {
            callData->args[0] = acc;
            callData->args[1] = v;
            callData->args[2] = Value::fromDouble(k);
            acc = callback->call(callData);
        }
        ++k;
    }
    return acc.asReturnedValue();
}

ReturnedValue ArrayPrototype::method_reduceRight(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Object *instance = ctx->thisObject.toObject(ctx);

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        ctx->throwTypeError();

    if (len == 0) {
        if (ctx->argumentCount == 1)
            ctx->throwTypeError();
        return ctx->argument(1).asReturnedValue();
    }

    uint k = len;
    ScopedValue acc(scope);
    ScopedValue v(scope);
    if (ctx->argumentCount > 1) {
        acc = ctx->argument(1);
    } else {
        bool kPresent = false;
        while (k > 0 && !kPresent) {
            v = instance->getIndexed(k - 1, &kPresent);
            if (kPresent)
                acc = v;
            --k;
        }
        if (!kPresent)
            ctx->throwTypeError();
    }

    ScopedCallData callData(scope, 4);
    callData->thisObject = Value::undefinedValue();
    callData->args[3] = Value::fromObject(instance);

    while (k > 0) {
        bool kPresent;
        v = instance->getIndexed(k - 1, &kPresent);
        if (kPresent) {
            callData->args[0] = acc;
            callData->args[1] = v;
            callData->args[2] = Value::fromDouble(k - 1);
            acc = callback->call(callData);
        }
        --k;
    }
    return acc.asReturnedValue();
}

