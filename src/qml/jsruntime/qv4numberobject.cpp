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

#include "qv4numberobject_p.h"
#include <QtCore/qnumeric.h>
#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <cassert>
#include <double-conversion.h>

using namespace QV4;

DEFINE_MANAGED_VTABLE(NumberCtor);

NumberCtor::NumberCtor(ExecutionContext *scope)
    : FunctionObject(scope, scope->engine->newIdentifier(QStringLiteral("Number")))
{
    vtbl = &static_vtbl;
}

ReturnedValue NumberCtor::construct(Managed *m, CallData *callData)
{
    double dbl = callData->argc ? callData->args[0].toNumber() : 0.;
    return Value::fromObject(m->engine()->newNumberObject(Value::fromDouble(dbl))).asReturnedValue();
}

ReturnedValue NumberCtor::call(Managed *, CallData *callData)
{
    double dbl = callData->argc ? callData->args[0].toNumber() : 0.;
    return Value::fromDouble(dbl).asReturnedValue();
}

void NumberPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));

    ctor.objectValue()->defineReadonlyProperty(ctx->engine, QStringLiteral("NaN"), Value::fromDouble(qSNaN()));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine, QStringLiteral("NEGATIVE_INFINITY"), Value::fromDouble(-qInf()));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine, QStringLiteral("POSITIVE_INFINITY"), Value::fromDouble(qInf()));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine, QStringLiteral("MAX_VALUE"), Value::fromDouble(1.7976931348623158e+308));

#ifdef __INTEL_COMPILER
# pragma warning( push )
# pragma warning(disable: 239)
#endif
    ctor.objectValue()->defineReadonlyProperty(ctx->engine, QStringLiteral("MIN_VALUE"), Value::fromDouble(5e-324));
#ifdef __INTEL_COMPILER
# pragma warning( pop )
#endif

    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString);
    defineDefaultProperty(ctx, QStringLiteral("toLocaleString"), method_toLocaleString);
    defineDefaultProperty(ctx, QStringLiteral("valueOf"), method_valueOf);
    defineDefaultProperty(ctx, QStringLiteral("toFixed"), method_toFixed, 1);
    defineDefaultProperty(ctx, QStringLiteral("toExponential"), method_toExponential);
    defineDefaultProperty(ctx, QStringLiteral("toPrecision"), method_toPrecision);
}

inline Value thisNumberValue(ExecutionContext *ctx)
{
    if (ctx->thisObject.isNumber())
        return ctx->thisObject;
    NumberObject *n = ctx->thisObject.asNumberObject();
    if (!n)
        ctx->throwTypeError();
    return n->value;
}

ReturnedValue NumberPrototype::method_toString(SimpleCallContext *ctx)
{
    double num = thisNumberValue(ctx).asDouble();

    Value arg = ctx->argument(0);
    if (!arg.isUndefined()) {
        int radix = arg.toInt32();
        if (radix < 2 || radix > 36) {
            ctx->throwError(QString::fromLatin1("Number.prototype.toString: %0 is not a valid radix")
                            .arg(radix));
            return Encode::undefined();
        }

        if (std::isnan(num)) {
            return Value::fromString(ctx, QStringLiteral("NaN")).asReturnedValue();
        } else if (qIsInf(num)) {
            return Value::fromString(ctx, QLatin1String(num < 0 ? "-Infinity" : "Infinity")).asReturnedValue();
        }

        if (radix != 10) {
            QString str;
            bool negative = false;
            if (num < 0) {
                negative = true;
                num = -num;
            }
            double frac = num - ::floor(num);
            num = Value::toInteger(num);
            do {
                char c = (char)::fmod(num, radix);
                c = (c < 10) ? (c + '0') : (c - 10 + 'a');
                str.prepend(QLatin1Char(c));
                num = ::floor(num / radix);
            } while (num != 0);
            if (frac != 0) {
                str.append(QLatin1Char('.'));
                do {
                    frac = frac * radix;
                    char c = (char)::floor(frac);
                    c = (c < 10) ? (c + '0') : (c - 10 + 'a');
                    str.append(QLatin1Char(c));
                    frac = frac - ::floor(frac);
                } while (frac != 0);
            }
            if (negative)
                str.prepend(QLatin1Char('-'));
            return Value::fromString(ctx, str).asReturnedValue();
        }
    }

    String *str = Value::fromDouble(num).toString(ctx);
    return Value::fromString(str).asReturnedValue();
}

ReturnedValue NumberPrototype::method_toLocaleString(SimpleCallContext *ctx)
{
    Value v = thisNumberValue(ctx);

    String *str = v.toString(ctx);
    return Value::fromString(str).asReturnedValue();
}

ReturnedValue NumberPrototype::method_valueOf(SimpleCallContext *ctx)
{
    return thisNumberValue(ctx).asReturnedValue();
}

ReturnedValue NumberPrototype::method_toFixed(SimpleCallContext *ctx)
{
    double v = thisNumberValue(ctx).asDouble();

    double fdigits = 0;

    if (ctx->argumentCount > 0)
        fdigits = ctx->argument(0).toInteger();

    if (std::isnan(fdigits))
        fdigits = 0;

    if (fdigits < 0 || fdigits > 20)
        ctx->throwRangeError(ctx->thisObject);

    QString str;
    if (std::isnan(v))
        str = QString::fromLatin1("NaN");
    else if (qIsInf(v))
        str = QString::fromLatin1(v < 0 ? "-Infinity" : "Infinity");
    else if (v < 1.e21)
        str = QString::number(v, 'f', int (fdigits));
    else
        return __qmljs_string_from_number(ctx, v);
    return Value::fromString(ctx, str).asReturnedValue();
}

ReturnedValue NumberPrototype::method_toExponential(SimpleCallContext *ctx)
{
    double d = thisNumberValue(ctx).asDouble();

    Value fraction = ctx->argument(0);
    int fdigits = -1;

    if (!fraction.isUndefined()) {
        int fdigits = ctx->argument(0).toInt32();
        if (fdigits < 0 || fdigits > 20) {
            String *error = ctx->engine->newString(QStringLiteral("Number.prototype.toExponential: fractionDigits out of range"));
            ctx->throwRangeError(Value::fromString(error));
        }
    }

    char str[100];
    double_conversion::StringBuilder builder(str, sizeof(str));
    double_conversion::DoubleToStringConverter::EcmaScriptConverter().ToExponential(d, fdigits, &builder);
    QString result = QString::fromLatin1(builder.Finalize());

    return Value::fromString(ctx, result).asReturnedValue();
}

ReturnedValue NumberPrototype::method_toPrecision(SimpleCallContext *ctx)
{
    Scope scope(ctx);

    ScopedValue v(scope, thisNumberValue(ctx));

    Value prec = ctx->argument(0);
    if (prec.isUndefined())
        return __qmljs_to_string(v, ctx);

    double precision = prec.toInt32();
    if (precision < 1 || precision > 21) {
        String *error = ctx->engine->newString(QStringLiteral("Number.prototype.toPrecision: precision out of range"));
        ctx->throwRangeError(Value::fromString(error));
    }

    char str[100];
    double_conversion::StringBuilder builder(str, sizeof(str));
    double_conversion::DoubleToStringConverter::EcmaScriptConverter().ToPrecision(v->asDouble(), precision, &builder);
    QString result = QString::fromLatin1(builder.Finalize());

    return Value::fromString(ctx, result).asReturnedValue();
}
