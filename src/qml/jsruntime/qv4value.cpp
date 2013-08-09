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
#include <qv4engine_p.h>
#include <qv4object_p.h>
#include <qv4objectproto_p.h>
#include "qv4mm_p.h"
#include "qv4exception_p.h"

#include <wtf/MathExtras.h>

using namespace QV4;

int Value::toUInt16() const
{
    if (isConvertibleToInt())
        return (ushort)(uint)integerValue();

    double number = __qmljs_to_number(*this);

    double D16 = 65536.0;
    if ((number >= 0 && number < D16))
        return static_cast<ushort>(number);

    if (!std::isfinite(number))
        return +0;

    double d = ::floor(::fabs(number));
    if (std::signbit(number))
        d = -d;

    number = ::fmod(d , D16);

    if (number < 0)
        number += D16;

    return (unsigned short)number;
}

double Value::toInteger() const
{
    if (isConvertibleToInt())
        return int_32;

    return Value::toInteger(__qmljs_to_number(*this));
}

double Value::toNumber() const
{
    return __qmljs_to_number(*this);
}

QString Value::toQString() const
{
    switch (type()) {
    case Value::Undefined_Type:
        return QStringLiteral("undefined");
    case Value::Null_Type:
        return QStringLiteral("null");
    case Value::Boolean_Type:
        if (booleanValue())
            return QStringLiteral("true");
        else
            return QStringLiteral("false");
    case Value::String_Type:
        return stringValue()->toQString();
    case Value::Object_Type: {
        ExecutionContext *ctx = objectValue()->internalClass->engine->current;
        try {
            Value prim = __qmljs_to_primitive(*this, STRING_HINT);
            if (prim.isPrimitive())
                return prim.toQString();
        } catch (Exception &e) {
            e.accept(ctx);
            try {
                Value prim = __qmljs_to_primitive(e.value(), STRING_HINT);
                if (prim.isPrimitive())
                    return prim.toQString();
            } catch(Exception &e) {
                e.accept(ctx);
            }
        }
        return QString();
    }
    case Value::Integer_Type: {
        QString str;
        __qmljs_numberToString(&str, (double)int_32, 10);
        return str;
    }
    default: { // double
        QString str;
        __qmljs_numberToString(&str, doubleValue(), 10);
        return str;
    }
    } // switch
}

bool Value::sameValue(Value other) const {
    if (val == other.val)
        return true;
    if (isString() && other.isString())
        return stringValue()->isEqualTo(other.stringValue());
    if (isInteger())
        return int_32 ? (double(int_32) == other.dbl) : (other.val == 0);
    if (other.isInteger())
        return other.int_32 ? (dbl == double(other.int_32)) : (val == 0);
    return false;
}

Value Value::fromString(ExecutionContext *ctx, const QString &s)
{
    return fromString(ctx->engine->newString(s));
}

Value Value::fromString(ExecutionEngine *engine, const QString &s)
{
    return fromString(engine->newString(s));
}


int Value::toInt32(double number)
{
    const double D32 = 4294967296.0;
    const double D31 = D32 / 2.0;

    if ((number >= -D31 && number < D31))
        return static_cast<int>(number);


    if (!std::isfinite(number))
        return 0;

    double d = ::floor(::fabs(number));
    if (std::signbit(number))
        d = -d;

    number = ::fmod(d , D32);

    if (number < -D31)
        number += D32;
    else if (number >= D31)
        number -= D32;

    return int(number);
}

unsigned int Value::toUInt32(double number)
{
    const double D32 = 4294967296.0;
    if ((number >= 0 && number < D32))
        return static_cast<uint>(number);

    if (!std::isfinite(number))
        return +0;

    double d = ::floor(::fabs(number));
    if (std::signbit(number))
        d = -d;

    number = ::fmod(d , D32);

    if (number < 0)
        number += D32;

    return unsigned(number);
}

double Value::toInteger(double number)
{
    if (std::isnan(number))
        return +0;
    else if (! number || std::isinf(number))
        return number;
    const double v = floor(fabs(number));
    return std::signbit(number) ? -v : v;
}

String *Value::toString(ExecutionContext *ctx) const
{
    if (isString())
        return stringValue();
    return __qmljs_convert_to_string(ctx, *this);
}

Value Value::property(ExecutionContext *ctx, String *name) const
{
    return isObject() ? objectValue()->get(name) : undefinedValue();
}


PersistentValue::PersistentValue(const Value &val)
    : d(new PersistentValuePrivate(val))
{
}

PersistentValue::PersistentValue(const PersistentValue &other)
    : d(other.d)
{
    if (d)
        d->ref();
}

PersistentValue &PersistentValue::operator=(const PersistentValue &other)
{
    if (d == other.d)
        return *this;

    // the memory manager cleans up those with a refcount of 0

    if (d)
        d->deref();
    d = other.d;
    if (d)
        d->ref();

    return *this;
}

PersistentValue &PersistentValue::operator =(const Value &other)
{
    if (!d) {
        d = new PersistentValuePrivate(other);
        return *this;
    }
    d = d->detach(other);
    return *this;
}

PersistentValue::~PersistentValue()
{
    if (d)
        d->deref();
}

WeakValue::WeakValue(const Value &val)
    : d(new PersistentValuePrivate(val, /*engine*/0, /*weak*/true))
{
}

WeakValue::WeakValue(const WeakValue &other)
    : d(other.d)
{
    if (d)
        d->ref();
}

WeakValue &WeakValue::operator=(const WeakValue &other)
{
    if (d == other.d)
        return *this;

    // the memory manager cleans up those with a refcount of 0

    if (d)
        d->deref();
    d = other.d;
    if (d)
        d->ref();

    return *this;
}

WeakValue &WeakValue::operator =(const Value &other)
{
    if (!d) {
        d = new PersistentValuePrivate(other, /*engine*/0, /*weak*/true);
        return *this;
    }
    d = d->detach(other, /*weak*/true);
    return *this;
}


WeakValue::~WeakValue()
{
    if (d)
        d->deref();
}

void WeakValue::markOnce()
{
    if (!d)
        return;
    Managed *m = d->value.asManaged();
    if (!m)
        return;
    m->mark();
}

PersistentValuePrivate::PersistentValuePrivate(const Value &v, ExecutionEngine *e, bool weak)
    : value(v)
    , refcount(1)
    , prev(0)
    , next(0)
    , engine(e)
{
    if (!engine) {
        Managed *m = v.asManaged();
        if (!m)
            return;

        engine = m->engine();
    }
    if (engine) {
        PersistentValuePrivate **listRoot = weak ? &engine->memoryManager->m_weakValues : &engine->memoryManager->m_persistentValues;

        prev = listRoot;
        next = *listRoot;
        *prev = this;
        if (next)
            next->prev = &this->next;
    }
}

PersistentValuePrivate::~PersistentValuePrivate()
{
}

void PersistentValuePrivate::removeFromList()
{
    if (prev) {
        if (next)
            next->prev = prev;
        *prev = next;
        next = 0;
        prev = 0;
    }
}

void PersistentValuePrivate::deref()
{
    // if engine is not 0, they are registered with the memory manager
    // and will get cleaned up in the next gc run
    if (!--refcount) {
        removeFromList();
        delete this;
    }
}

PersistentValuePrivate *PersistentValuePrivate::detach(const QV4::Value &value, bool weak)
{
    if (refcount == 1) {
        this->value = value;

        Managed *m = value.asManaged();
        if (!prev) {
            if (m) {
                ExecutionEngine *engine = m->engine();
                if (engine) {
                    PersistentValuePrivate **listRoot = weak ? &engine->memoryManager->m_weakValues : &engine->memoryManager->m_persistentValues;
                    prev = listRoot;
                    next = *listRoot;
                    *prev = this;
                    if (next)
                        next->prev = &this->next;
                }
            }
        } else if (!m)
            removeFromList();

        return this;
    }
    --refcount;
    return new PersistentValuePrivate(value, engine, weak);
}
