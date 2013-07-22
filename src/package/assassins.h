#ifndef ASSASSINSPACKAGE_H
#define ASSASSINSPACKAGE_H

#include "package.h"
#include "card.h"
#include "skill.h"
#include "standard.h"

class AssassinsPackage : public Package{
    Q_OBJECT

public:
    AssassinsPackage();
};

class FengyinCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE FengyinCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class MixinCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE MixinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class DuyiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE DuyiCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

//Olympics

class OlympicsPackage : public Package{
    Q_OBJECT

public:
    OlympicsPackage();
};

class JisuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE JisuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class XintanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE XintanCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

#endif // ASSASSINSPACKAGE_H
