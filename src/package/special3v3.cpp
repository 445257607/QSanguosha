#include "special3v3.h"
#include "skill.h"
#include "standard.h"
#include "server.h"
#include "carditem.h"
#include "engine.h"
#include "maneuvering.h"

HongyuanCard::HongyuanCard(){
    mute = true;
}

bool HongyuanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(to_select == Self)
        return false;
    return targets.length() < 2;
}

void HongyuanCard::onEffect(const CardEffectStruct &effect) const{
   effect.to->drawCards(1);
}

class HongyuanViewAsSkill: public ZeroCardViewAsSkill{
public:
    HongyuanViewAsSkill():ZeroCardViewAsSkill("hongyuan"){
    }

    virtual const Card *viewAs() const{
        return new HongyuanCard;
    }

protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@hongyuan";
    }
};

class Hongyuan:public DrawCardsSkill{
public:
    Hongyuan():DrawCardsSkill("hongyuan"){
        frequency = NotFrequent;
        view_as_skill = new HongyuanViewAsSkill;
    }

    virtual int getDrawNum(ServerPlayer *zhugejin, int n) const{
        Room *room = zhugejin->getRoom();
        if(room->askForSkillInvoke(zhugejin, objectName())){
            room->playSkillEffect(objectName());
            room->setPlayerFlag(zhugejin, "Invoked");
            return n - 1;
        }else
            return n;
    }
};

class HongyuanAct: public TriggerSkill{
public:
    HongyuanAct():TriggerSkill("#hongyuan"){
        events << CardDrawnDone;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *zhugejin, QVariant &data) const{
        if(zhugejin->getPhase() == Player::Draw && zhugejin->hasFlag("Invoked")){
            room->setPlayerFlag(zhugejin, "-Invoked");
            if(ServerInfo.GameMode == "06_3v3"){
                foreach(ServerPlayer *other, room->getOtherPlayers(zhugejin)){
                    if(Sanguosha->is3v3Friend(zhugejin, other))
                        other->drawCards(1);
                }

            }else{
                if(!room->askForUseCard(zhugejin, "@@hongyuan", "@hongyuan"))
                   zhugejin->drawCards(1);
            }
        }
        return false;
    }
};

HuanshiCard::HuanshiCard(){
    target_fixed = true;
    will_throw = false;
    can_jilei = true;
}

void HuanshiCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{

}

class HuanshiViewAsSkill:public OneCardViewAsSkill{
public:
    HuanshiViewAsSkill():OneCardViewAsSkill("huanshi"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@huanshi";
    }

    virtual bool viewFilter(const CardItem *) const{
        return true;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        Card *card = new HuanshiCard;
        card->setSuit(card_item->getFilteredCard()->getSuit());
        card->addSubcard(card_item->getFilteredCard());

        return card;
    }
};

class Huanshi: public TriggerSkill{
public:
    Huanshi():TriggerSkill("huanshi"){
        view_as_skill = new HuanshiViewAsSkill;

        events << AskForRetrial;
    }

    QList<ServerPlayer *> getTeammates(ServerPlayer *zhugejin) const{
        Room *room = zhugejin->getRoom();

        QList<ServerPlayer *> teammates;
        foreach(ServerPlayer *other, room->getOtherPlayers(zhugejin)){
            if(Sanguosha->is3v3Friend(zhugejin, other))
                teammates << other;
        }
        return teammates;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && !target->isNude();
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();

        bool can_invoke = false;
        if(ServerInfo.GameMode == "06_3v3"){
            foreach(ServerPlayer *teammate, getTeammates(player)){
                if(teammate->objectName() == judge->who->objectName()){
                    can_invoke = true;
                    break;
                }
            }
        }else if(judge->who->objectName() != player->objectName()){
            if(room->askForSkillInvoke(player,objectName()))
                if(room->askForChoice(judge->who, objectName(), "yes+no") == "yes")
                    can_invoke = true;
        }
        else
            can_invoke = true;
        if(!can_invoke)
            return false;
        QStringList prompt_list;
        prompt_list << "@huanshi-card" << judge->who->objectName()
                << objectName() << judge->reason << judge->card->getEffectIdString();
        QString prompt = prompt_list.join(":");

        player->tag["Judge"] = data;
        room->setPlayerFlag(player, "retrial");
        const Card *card = room->askForCard(player, "@huanshi", prompt, data);

        if(card){
            // the only difference for Guicai & Guidao
            room->throwCard(judge->card, judge->who);

            judge->card = Sanguosha->getCard(card->getEffectiveId());

            room->moveCardTo(judge->card, NULL, Player::Special);
            LogMessage log;
            log.type = "$ChangedJudge";
            log.from = player;
            log.to << judge->who;
            log.card_str = card->getEffectIdString();
            room->sendLog(log);

            room->sendJudgeResult(judge);
        }
        else
            room->setPlayerFlag(player, "-retrial");

        return false;
    }
};

class Mingzhe: public TriggerSkill{
public:
    Mingzhe():TriggerSkill("mingzhe"){
        events << CardLost;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::NotActive)
            return false;

        CardMoveStar move = data.value<CardMoveStar>();
        const Card *card = Sanguosha->getCard(move->card_id);
        if(card->isRed() && player->askForSkillInvoke(objectName(), data)){
            room->playSkillEffect(objectName());
            player->drawCards(1);
        }
        return false;
    }
};

class Zhenvei: public DistanceSkill{
public:
    Zhenvei():DistanceSkill("zhenvei"){

    }

    static bool isFriend(const Player *a, const Player *b){
        if(ServerInfo.GameMode == "06_3v3"){
            QChar c = a->getRole().at(0);
            return b->getRole().startsWith(c);
        }
        else
            return b->getKingdom() == a->getKingdom();
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        const Player *wenpin = NULL;
        QList<const Player *> players = from->getSiblings();
        players << from;
        foreach(const Player *player, players){
            if(player->isAlive() && player->hasSkill(objectName())){
                wenpin = player;
                players.removeOne(player);
                break;
            }
        }
        if(wenpin && to != wenpin &&
           isFriend(wenpin, to) &&
           !isFriend(wenpin, from))
            return +1;
        else
            return 0;
    }
};

XZhongyiCard::XZhongyiCard(){
    target_fixed = true;
    mute = true;
}

void XZhongyiCard::use(Room *, ServerPlayer *player, const QList<ServerPlayer *> &) const{
    player->playSkillEffect(skill_name, 1);
    player->loseAllMarks("@honest");
    player->addToPile("honest", getSubcards().first());
}

class XZhongyiViewAsSkill: public OneCardViewAsSkill{
public:
    XZhongyiViewAsSkill():OneCardViewAsSkill("x~zhongyi"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->hasMark("@honest");
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return !to_select->isEquipped() && to_select->getCard()->isRed();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        Card *card = new XZhongyiCard;
        card->addSubcard(card_item->getFilteredCard());
        return card;
    }
};

class XZhongyi: public TriggerSkill{
public:
    XZhongyi():TriggerSkill("x~zhongyi"){
        view_as_skill = new XZhongyiViewAsSkill;
        events << Predamage << PhaseChange << GameStart;
        frequency = Limited;
    }

    virtual bool triggerable(const ServerPlayer *i) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
        if(ServerInfo.GameMode != "06_3v3")
            return false;
        if(event == GameStart){
            if(player->hasSkill(objectName()))
                player->gainMark("@honest");
            return false;
        }
        if(event == PhaseChange){
            if(player->hasSkill(objectName()) && player->getPhase() == Player::RoundStart
               && !player->hasFlag("actioned"))
                player->clearPile("honest");
            return false;
        }
        ServerPlayer *guanyu = room->findPlayerBySkillName(objectName());
        if(!guanyu || guanyu->getPile("honest").isEmpty())
            return false;
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.card && damage.card->inherits("Slash") &&
           Sanguosha->is3v3Friend(guanyu, player)){

            LogMessage log;
            log.type = "#XZhongyi";
            log.from = player;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(damage.damage + 1);
            room->sendLog(log);

            damage.damage ++;
            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class XJiuzhu: public TriggerSkill{
public:
    XJiuzhu():TriggerSkill("x~jiuzhu"){
        events << Dying;
    }

    virtual bool triggerable(const ServerPlayer *) const{
        return true;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(ServerInfo.GameMode != "06_3v3")
            return false;
        QList<ServerPlayer *> zhaoyuns = room->findPlayersBySkillName(objectName());
        foreach(ServerPlayer *zhaoyun, zhaoyuns){
            if(player->getHp() > 0)
                break;
            if(zhaoyun->getHp() == 1)
                continue;
            if(!Sanguosha->is3v3Friend(zhaoyun, player))
                continue;
            if(!zhaoyun->isNude() && zhaoyun->askForSkillInvoke(objectName(), data)){
                room->playSkillEffect(objectName());
                room->loseHp(zhaoyun);
                room->askForDiscard(zhaoyun, objectName(), 1, false, true);

                RecoverStruct recover;
                recover.who = zhaoyun;
                room->recover(player, recover, true);
            }
        }
        return false;
    }
};

class XZhanshen: public PhaseChangeSkill{
public:
    XZhanshen():PhaseChangeSkill("x~zhanshen"){
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
                && target->getMark("xzhanshen") == 0
                && target->getPhase() == Player::Start
                && target->isWounded();
    }

    virtual bool onPhaseChange(ServerPlayer *lvbu) const{
        if(ServerInfo.GameMode != "06_3v3")
            return false;
        Room *room = lvbu->getRoom();
        int x = 0;
        foreach(ServerPlayer *fri, room->getOtherPlayers(lvbu))
            if(Sanguosha->is3v3Friend(lvbu, fri))
                x ++;
        if(x >= 2)
            return false;

        LogMessage log;
        log.type = "#WakeUp";
        log.from = lvbu;
        log.arg = "xzhanshen";
        room->sendLog(log);

        //room->playLightbox(lvbu, "x~zhanshen", "5000", 5000);
        room->loseMaxHp(lvbu);
        room->throwCard(lvbu->getWeapon(), lvbu);

        room->acquireSkill(lvbu, "mashu");
        room->acquireSkill(lvbu, "shenji");

        room->setPlayerMark(lvbu, "xzhanshen", 1);

        return false;
    }
};

Special3v3Package::Special3v3Package():Package("Special3v3")
{
    skills << new XJiuzhu << new XZhongyi << new XZhanshen;
    addMetaObject<XZhongyiCard>();

    General *zhugejin = new General(this, "zhugejin", "wu", 3, true);
    zhugejin->addSkill(new Huanshi);
    zhugejin->addSkill(new Hongyuan);
    zhugejin->addSkill(new HongyuanAct);
    zhugejin->addSkill(new Mingzhe);
    related_skills.insertMulti("hongyuan", "#hongyuan");

    addMetaObject<HuanshiCard>();
    addMetaObject<HongyuanCard>();

    General *wenpin = new General(this, "wenpin", "wei");
    wenpin->addSkill(new Zhenvei);
}

New3v3CardPackage::New3v3CardPackage()
    :Package("New3v3Card")
{
    QList<Card *> cards;
    cards << new SupplyShortage(Card::Spade, 1)
          << new SupplyShortage(Card::Club, 12)
          << new Nullification(Card::Heart, 12);

    foreach(Card *card, cards)
        card->setParent(this);

    type = CardPack;
}

ADD_PACKAGE(Special3v3)
ADD_PACKAGE(New3v3Card)
