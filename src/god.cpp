#include "god.h"
#include "client.h"
#include "engine.h"
#include "carditem.h"

GongxinCard::GongxinCard(){
    once = true;
}

bool GongxinCard::targetFilter(const QList<const ClientPlayer *> &targets, const ClientPlayer *to_select) const{
    return targets.isEmpty() && !to_select->isKongcheng();
}

void GongxinCard::onEffect(const CardEffectStruct &effect) const{
    effect.from->getRoom()->doGongxin(effect.from, effect.to);
}

class Wuhun: public TriggerSkill{
public:
    Wuhun():TriggerSkill("wuhun"){
        events << DamageDone;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        if(damage.from)
            damage.from->gainMark("@nightmare", damage.damage);

        return false;
    }
};

static QString WuhunCallback(const Card *card, Room *){
    if(card->inherits("Peach") || card->inherits("GodSalvation"))
        return "good";
    else
        return "bad";
}

class WuhunRevenge: public TriggerSkill{
public:
    WuhunRevenge():TriggerSkill("#wuhun"){
        events << Death;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill("wuhun");
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *shenguanyu, QVariant &) const{
        Room *room = shenguanyu->getRoom();
        QList<ServerPlayer *> players = room->getOtherPlayers(shenguanyu);
        players << shenguanyu;

        int max = 0;
        foreach(ServerPlayer *player, players){
            int value = player->getMark("@nightmare");
            if(value > max)
                max = value;
        }

        if(max == 0)
            return false;

        QList<ServerPlayer *> foes;
        foreach(ServerPlayer *player, players){
            if(player->getMark("@nightmare") == max)
                foes << player;
        }

        if(foes.isEmpty())
            return false;

        ServerPlayer *foe;
        if(foes.length() == 1)
            foe = foes.first();
        else
            foe = room->askForPlayerChosen(shenguanyu, foes, "wuhun");

        if(room->judge(foe, WuhunCallback) == "bad" && foe != shenguanyu)
            room->killPlayer(foe);

        return false;
    }
};

static bool CompareBySuit(int card1, int card2){
    const Card *c1 = Sanguosha->getCard(card1);
    const Card *c2 = Sanguosha->getCard(card2);

    int a = static_cast<int>(c1->getSuit());
    int b = static_cast<int>(c2->getSuit());

    return a < b;
}

class Shelie: public PhaseChangeSkill{
public:
    Shelie():PhaseChangeSkill("shelie"){

    }

    virtual bool onPhaseChange(ServerPlayer *shenlumeng) const{
        if(shenlumeng->getPhase() != Player::Draw)
            return false;

        Room *room = shenlumeng->getRoom();
        if(!shenlumeng->askForSkillInvoke(objectName()))
            return false;

        room->playSkillEffect(objectName());

        QList<int> card_ids = room->getNCards(5);
        qSort(card_ids.begin(), card_ids.end(), CompareBySuit);        
        room->fillAG(card_ids);

        while(!card_ids.isEmpty()){
            int card_id = room->askForAG(shenlumeng, card_ids);
            card_ids.removeOne(card_id);
            room->takeAG(shenlumeng, card_id);

            // throw the rest cards that matches the same suit
            const Card *card = Sanguosha->getCard(card_id);
            Card::Suit suit = card->getSuit();
            QMutableListIterator<int> itor(card_ids);
            while(itor.hasNext()){
                const Card *c = Sanguosha->getCard(itor.next());
                if(c->getSuit() == suit){
                    itor.remove();

                    room->takeAG(NULL, c->getId());
                }
            }
        }

        room->broadcastInvoke("clearAG");

        return true;
    }
};

class Gongxin: public ZeroCardViewAsSkill{
public:
    Gongxin():ZeroCardViewAsSkill("gongxin"){
        default_choice = "discard";
    }

    virtual const Card *viewAs() const{
        return new GongxinCard;
    }

    virtual bool isEnabledAtPlay() const{
        return !ClientInstance->hasUsed("GongxinCard");
    }
};

void YeyanCard::damage(ServerPlayer *shenzhouyu, ServerPlayer *target, int point) const{
    DamageStruct damage;

    damage.card = NULL;
    damage.from = shenzhouyu;
    damage.to = target;
    damage.damage = point;
    damage.nature = DamageStruct::Fire;

    shenzhouyu->getRoom()->damage(damage);
}

GreatYeyanCard::GreatYeyanCard(){

}

bool GreatYeyanCard::targetFilter(const QList<const ClientPlayer *> &targets, const ClientPlayer *to_select) const{
    return targets.isEmpty();
}

void GreatYeyanCard::use(Room *room, ServerPlayer *shenzhouyu, const QList<ServerPlayer *> &targets) const{
    room->broadcastInvoke("animate", "lightbox:$greatyeyan");

    shenzhouyu->loseMark("@flame");
    room->throwCard(this);
    room->loseHp(shenzhouyu, 3);

    damage(shenzhouyu, targets.first(), 3);
}

MediumYeyanCard::MediumYeyanCard(){

}

bool MediumYeyanCard::targetFilter(const QList<const ClientPlayer *> &targets, const ClientPlayer *to_select) const{
    return targets.length() < 2;
}

void MediumYeyanCard::use(Room *room, ServerPlayer *shenzhouyu, const QList<ServerPlayer *> &targets) const{
    room->broadcastInvoke("animate", "lightbox:$mediumyeyan");

    shenzhouyu->loseMark("@flame");
    room->throwCard(this);
    room->loseHp(shenzhouyu, 3);

    ServerPlayer *first = targets.first();
    ServerPlayer *second = targets.value(1, NULL);

    damage(shenzhouyu, first, 2);

    if(second)
        damage(shenzhouyu, second, 1);
}

SmallYeyanCard::SmallYeyanCard(){

}

bool SmallYeyanCard::targetFilter(const QList<const ClientPlayer *> &targets, const ClientPlayer *to_select) const{
    return targets.length() < 3;
}

void SmallYeyanCard::use(Room *room, ServerPlayer *shenzhouyu, const QList<ServerPlayer *> &targets) const{
    room->broadcastInvoke("animate", "lightbox:$smallyeyan");
    shenzhouyu->loseMark("@flame");

    foreach(ServerPlayer *target, targets)
        room->cardEffect(this, shenzhouyu, target);
}

void SmallYeyanCard::onEffect(const CardEffectStruct &effect) const{
    damage(effect.from, effect.to, 1);
}

class GreatYeyan: public ViewAsSkill{
public:
    GreatYeyan(): ViewAsSkill("greatyeyan"){
        frequency = Limited;
    }

    virtual bool isEnabledAtPlay() const{
        return Self->getMark("@flame") >= 1;
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        if(selected.length() >= 4)
            return false;

        foreach(CardItem *item, selected){
            if(to_select->getFilteredCard()->getSuit() == item->getFilteredCard()->getSuit())
                return false;
        }

        return true;
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() != 4)
            return NULL;

        GreatYeyanCard *card = new GreatYeyanCard;
        card->addSubcards(cards);

        return card;
    }
};

class MediumYeyan: public GreatYeyan{
public:
    MediumYeyan(){
        setObjectName("mediumyeyan");
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() != 4)
            return NULL;

        MediumYeyanCard *card = new MediumYeyanCard;
        card->addSubcards(cards);

        return card;
    }
};

class SmallYeyan: public ZeroCardViewAsSkill{
public:
    SmallYeyan():ZeroCardViewAsSkill("smallyeyan"){
        frequency = Limited;
    }

    virtual bool isEnabledAtPlay() const{
        return Self->getMark("@flame") >= 1;
    }

    virtual const Card *viewAs() const{
        return new SmallYeyanCard;
    }
};

class Qinyin: public TriggerSkill{
public:
    Qinyin():TriggerSkill("qinyin"){
        events << CardDiscarded;
        default_choice = "down";
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *shenzhouyu, QVariant &data) const{
        CardStar dummy_card = data.value<CardStar>();
        int discard_num = dummy_card->subcardsLength();

        if(shenzhouyu->getPhase() == Player::Discard && discard_num >= 2){
            Room *room = shenzhouyu->getRoom();

            if(!shenzhouyu->askForSkillInvoke(objectName()))
                return false;

            QString result = room->askForChoice(shenzhouyu, objectName(), "up+down");
            QList<ServerPlayer *> all_players = room->getAllPlayers();
            if(result == "up"){
                room->playSkillEffect(objectName(), 2);
                foreach(ServerPlayer *player, all_players){
                    RecoverStruct recover;
                    recover.who = shenzhouyu;
                    room->recover(player, recover);
                }
            }else if(result == "down"){
                foreach(ServerPlayer *player, all_players){
                    room->loseHp(player);
                }

                int index = 1;
                if(room->findPlayer("caocao+shencaocao+shencc"))
                    index = 3;

                room->playSkillEffect(objectName(), index);
            }
        }

        return false;
    }
};

class Guixin: public MasochismSkill{
public:
    Guixin():MasochismSkill("guixin"){

    }

    virtual void onDamaged(ServerPlayer *shencc, const DamageStruct &damage) const{
        Room *room = shencc->getRoom();
        int i, x = damage.damage;
        for(i=0; i<x; i++){
            if(shencc->askForSkillInvoke(objectName())){
                room->playSkillEffect(objectName());

                QList<ServerPlayer *> players = room->getOtherPlayers(shencc);
                if(players.length() >=5)
                    room->broadcastInvoke("animate", "lightbox:$guixin");

                foreach(ServerPlayer *player, players){
                    if(!player->isAllNude()){
                        int card_id = room->askForCardChosen(shencc, player, "hej", objectName());
                        if(room->getCardPlace(card_id) == Player::Hand)
                            room->moveCardTo(Sanguosha->getCard(card_id), shencc, Player::Hand, false);
                        else
                            room->obtainCard(shencc, card_id);
                    }
                }

                shencc->turnOver();
            }else
                break;
        }
    }
};

Feiying::Feiying()
    :Skill("feiying", Skill::Compulsory)
{
}

class Kuangbao: public TriggerSkill{
public:
    Kuangbao():TriggerSkill("kuangbao"){
        events << Damage << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        LogMessage log;
        log.type = event == Damage ? "#KuangbaoDamage" : "#KuangbaoDamaged";
        log.from = player;
        log.arg = QString::number(damage.damage);
        player->getRoom()->sendLog(log);

        player->gainMark("@wrath", damage.damage);

        return false;
    }
};

class Wumou:public TriggerSkill{
public:
    Wumou():TriggerSkill("wumou"){
        frequency = Compulsory;
        events << CardUsed << CardResponsed;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        CardStar card = NULL;
        if(event == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
        }else if(event == CardResponsed)
            card = data.value<CardStar>();

        if(card->inherits("TrickCard") && !card->inherits("DelayedTrick")){
            Room *room = player->getRoom();
            int num = player->getMark("@wrath");
            if(num >= 1 && room->askForChoice(player, objectName(), "discard+losehp") == "discard"){
                player->loseMark("@wrath");
            }else
                room->loseHp(player);
        }

        return false;
    }
};

class Shenfen:public ZeroCardViewAsSkill{
public:
    Shenfen():ZeroCardViewAsSkill("shenfen"){
    }

    virtual bool isEnabledAtPlay() const{
        return Self->getMark("@wrath") >= 6 && !ClientInstance->hasUsed("ShenfenCard");
    }

    virtual const Card *viewAs() const{
        return new ShenfenCard;
    }
};


ShenfenCard::ShenfenCard(){
    target_fixed = true;
    once = true;
}

void ShenfenCard::use(Room *room, ServerPlayer *shenlubu, const QList<ServerPlayer *> &) const{
    shenlubu->loseMark("@wrath", 6);

    QList<ServerPlayer *> players = room->getOtherPlayers(shenlubu);
    foreach(ServerPlayer *player, players){
        DamageStruct damage;
        damage.card = this;
        damage.from = shenlubu;
        damage.to = player;

        room->damage(damage);
    }

    foreach(ServerPlayer *player, players){
        player->throwAllEquips();
    }

    foreach(ServerPlayer *player, players){
        if(player->getHandcardNum() <= 4)
            player->throwAllHandCards();
        else
            room->askForDiscard(player, "shenfen", 4);
    }

    shenlubu->turnOver();
}

WuqianCard::WuqianCard(){

}

bool WuqianCard::targetFilter(const QList<const ClientPlayer *> &targets, const ClientPlayer *to_select) const{
    return targets.isEmpty();
}

void WuqianCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    effect.from->loseMark("@wrath", 2);
    room->acquireSkill(effect.from, "wushuang", false);
    effect.from->setFlags("wuqian_used");

    effect.to->addMark("qinggang");
}

class WuqianViewAsSkill: public ZeroCardViewAsSkill{
public:
    WuqianViewAsSkill():ZeroCardViewAsSkill("wuqian"){

    }

    virtual bool isEnabledAtPlay() const{
        return Self->getMark("@wrath") >= 2;
    }

    virtual const Card *viewAs() const{
        return new WuqianCard;
    }
};

class Wuqian: public PhaseChangeSkill{
public:
    Wuqian():PhaseChangeSkill("wuqian"){
        view_as_skill = new WuqianViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *shenlubu) const{
        if(shenlubu->getPhase() == Player::NotActive){
            Room *room = shenlubu->getRoom();
            if(shenlubu->hasFlag("wuqian_used")){
                shenlubu->setFlags("-wuqian_used");
                QList<ServerPlayer *> players = room->getAllPlayers();
                foreach(ServerPlayer *player, players){
                    player->removeMark("qinggang");
                }

                const General *general2 = shenlubu->getGeneral2();
                if(general2 == NULL || !general2->hasSkill("wushuang"))
                    shenlubu->loseSkill("wushuang");
            }
        }

        return false;
    }
};

WushenSlash::WushenSlash(Card::Suit suit, int number)
    :Slash(suit, number)
{

}

class Wushen: public FilterSkill{
public:
    Wushen():FilterSkill("wushen"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getCard()->getSuit() == Card::Heart;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        WushenSlash *slash = new WushenSlash(card->getSuit(), card->getNumber());
        slash->addSubcard(card_item->getCard()->getId());
        slash->setSkillName(objectName());

        return slash;
    }
};

class Qixing: public PhaseChangeSkill{
public:
    Qixing():PhaseChangeSkill("qixing"){
        frequency = Frequent;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target) && target->getMark("@star") > 0;
    }

    static void Exchange(ServerPlayer *shenzhuge){
        QList<int> &stars = shenzhuge->getPile("stars");
        if(stars.isEmpty())
            return;

        Room *room = shenzhuge->getRoom();
        room->fillAG(stars, shenzhuge);

        QList<int> to_exchange;
        while(!stars.isEmpty()){
            int card_id = room->askForAG(shenzhuge, stars, true);
            if(card_id == -1)
                break;

            stars.removeOne(card_id);
            to_exchange << card_id;

            const Card *card = Sanguosha->getCard(card_id);
            shenzhuge->addCard(card, Player::Hand);
            room->setCardMapping(card_id, shenzhuge, Player::Hand);
            QString take_str = QString("%1:%2").arg(shenzhuge->objectName()).arg(card_id);
            shenzhuge->invoke("takeAG", take_str);

            shenzhuge->invoke("pile", QString("%1:stars-%2").arg(shenzhuge->objectName()).arg(card_id));
        }

        shenzhuge->invoke("clearAG");

        if(to_exchange.isEmpty())
            return;

        int n = to_exchange.length();
        const Card *exchange_card = room->askForExchange(shenzhuge, "qixing", n);

        foreach(int card_id, exchange_card->getSubcards()){
            shenzhuge->invoke("pile", QString("%1:stars+%2").arg(shenzhuge->objectName()).arg(card_id));
        }

        LogMessage log;
        log.type = "#QixingExchange";
        log.from = shenzhuge;
        log.arg = QString::number(n);
        room->sendLog(log);

        room->moveCardTo(exchange_card, shenzhuge, Player::Special, false);

        stars.append(exchange_card->getSubcards());

        delete exchange_card;
    }

    static void DiscardStar(ServerPlayer *shenzhuge, int n){
        Room *room = shenzhuge->getRoom();
        QList<int> &stars = shenzhuge->getPile("stars");

        room->fillAG(stars, shenzhuge);

        int i;
        for(i=0; i<n; i++){
            int card_id = room->askForAG(shenzhuge, stars);

            stars.removeOne(card_id);
            room->throwCard(card_id);

            shenzhuge->invoke("pile", QString("%1:stars-%2").arg(shenzhuge->objectName()).arg(card_id));
        }

        shenzhuge->invoke("clearAG");
    }

    virtual bool onPhaseChange(ServerPlayer *shenzhuge) const{
        if(shenzhuge->getPhase() == Player::Draw){
            Exchange(shenzhuge);
        }

        return false;
    }
};

class QixingStart: public GameStartSkill{
public:
    QixingStart():GameStartSkill("#qixing"){

    }

    virtual int getPriority() const{
        return -1;
    }

    virtual void onGameStart(ServerPlayer *shenzhuge) const{
        shenzhuge->gainMark("@star", 7);

        Room *room = shenzhuge->getRoom();
        QList<int> stars = room->getNCards(7);
        foreach(int star, stars)
            room->setCardMapping(star, shenzhuge, Player::Special);
        shenzhuge->getPile("stars") = stars;

        foreach(int star, stars){
            shenzhuge->invoke("pile", QString("%1:stars+%2").arg(shenzhuge->objectName()).arg(star));
        }

        Qixing::Exchange(shenzhuge);
    }
};

KuangfengCard::KuangfengCard(){

}

bool KuangfengCard::targetFilter(const QList<const ClientPlayer *> &targets, const ClientPlayer *to_select) const{
    return targets.isEmpty();
}

void KuangfengCard::onEffect(const CardEffectStruct &effect) const{
    Qixing::DiscardStar(effect.from, 1);

    effect.from->loseMark("@star");
    effect.to->gainMark("@gale");
}

class KuangfengViewAsSkill: public ZeroCardViewAsSkill{
public:
    KuangfengViewAsSkill():ZeroCardViewAsSkill("kuangfeng"){

    }

    virtual bool isEnabledAtPlay() const{
        return false;
    }

    virtual bool isEnabledAtResponse() const{
        return ClientInstance->card_pattern == "@kuangfeng";
    }

    virtual const Card *viewAs() const{
        return new KuangfengCard;
    }
};

class Kuangfeng: public TriggerSkill{
public:
    Kuangfeng():TriggerSkill("kuangfeng"){
        view_as_skill = new KuangfengViewAsSkill;

        events << Predamaged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getMark("@gale") > 0;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.nature == DamageStruct::Fire){
            LogMessage log;
            log.type = "#GalePower";
            log.from = player;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(damage.damage + 1);
            player->getRoom()->sendLog(log);

            damage.damage ++;
            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class QixingAsk: public PhaseChangeSkill{
public:
    QixingAsk():PhaseChangeSkill("#qixing-ask"){

    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        if(target->getPhase() == Player::Finish){
            if(target->getMark("@star") > 0)
                room->askForUseCard(target, "@kuangfeng", "@@kuangfeng-card");

            if(target->getMark("@star") > 0)
                room->askForUseCard(target, "@dawu", "@@dawu-card");
        }else if(target->getPhase() == Player::Start){
            QList<ServerPlayer *> players = room->getAllPlayers();
            foreach(ServerPlayer *player, players){
                if(player->getMark("@gale") > 0)
                    player->loseMark("@gale");
                if(player->getMark("@fog") > 0)
                    player->loseMark("@fog");
            }
        }

        return false;
    }
};

class QixingClear: public TriggerSkill{
public:
    QixingClear():TriggerSkill("#qixing-clear"){
        events << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> players = room->getAllPlayers();
        foreach(ServerPlayer *player, players){
            player->loseAllMarks("@gale");
            player->loseAllMarks("@fog");
        }

        return false;
    }
};

DawuCard::DawuCard(){

}

bool DawuCard::targetFilter(const QList<const ClientPlayer *> &targets, const ClientPlayer *to_select) const{
    return targets.length() < Self->getMark("@star");
}

void DawuCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    int n = targets.length();
    Qixing::DiscardStar(source, n);

    source->loseMark("@star", n);
    foreach(ServerPlayer *target, targets){
        target->gainMark("@fog");
    }
}

class DawuViewAsSkill: public ZeroCardViewAsSkill{
public:
    DawuViewAsSkill():ZeroCardViewAsSkill("dawu"){

    }

    virtual bool isEnabledAtPlay() const{
        return false;
    }

    virtual bool isEnabledAtResponse() const{
        return ClientInstance->card_pattern == "@dawu";
    }

    virtual const Card *viewAs() const{
        return new DawuCard;
    }
};

class Dawu: public TriggerSkill{
public:
    Dawu():TriggerSkill("dawu"){
        view_as_skill = new DawuViewAsSkill;

        events << Predamaged;
    }

    virtual int getPriority() const{
        return 2;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getMark("@fog") > 0;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.nature != DamageStruct::Thunder){
            Room *room = player->getRoom();

            LogMessage log;
            log.type = "#FogProtect";
            log.from = player;
            log.arg = QString::number(damage.damage);
            if(damage.nature == DamageStruct::Normal)
                log.arg2 = "normal_nature";
            else if(damage.nature == DamageStruct::Fire)
                log.arg2 = "fire_nature";
            room->sendLog(log);

            return true;
        }else
            return false;
    }
};

GodPackage::GodPackage()
    :Package("god")
{
    General *shenguanyu = new General(this, "shenguanyu", "god", 5);
    shenguanyu->addSkill(new Wushen);
    shenguanyu->addSkill(new Wuhun);
    shenguanyu->addSkill(new WuhunRevenge);

    General *shenlumeng = new General(this, "shenlumeng", "god", 3);
    shenlumeng->addSkill(new Shelie);
    shenlumeng->addSkill(new Gongxin);

    General *shenzhouyu = new General(this, "shenzhouyu", "god");
    shenzhouyu->addSkill(new Qinyin);
    shenzhouyu->addSkill(new MarkAssignSkill("@flame", 1));
    shenzhouyu->addSkill(new GreatYeyan);
    shenzhouyu->addSkill(new MediumYeyan);
    shenzhouyu->addSkill(new SmallYeyan);

    General *shenzhugeliang = new General(this, "shenzhugeliang", "god", 3);
    shenzhugeliang->addSkill(new Qixing);
    shenzhugeliang->addSkill(new QixingStart);
    shenzhugeliang->addSkill(new QixingAsk);
    shenzhugeliang->addSkill(new QixingClear);
    shenzhugeliang->addSkill(new Kuangfeng);
    shenzhugeliang->addSkill(new Dawu);

    General *shencaocao = new General(this, "shencaocao$", "god", 3);
    shencaocao->addSkill(new Guixin);
    shencaocao->addSkill(new Feiying);

    General *shenlubu = new General(this, "shenlubu", "god", 5);
    shenlubu->addSkill(new Kuangbao);
    shenlubu->addSkill(new MarkAssignSkill("@wrath", 2));
    shenlubu->addSkill(new Wumou);
    shenlubu->addSkill(new Wuqian);
    shenlubu->addSkill(new Shenfen);

    addMetaObject<GongxinCard>();
    addMetaObject<GreatYeyanCard>();
    addMetaObject<ShenfenCard>();
    addMetaObject<GreatYeyanCard>();
    addMetaObject<MediumYeyanCard>();
    addMetaObject<SmallYeyanCard>();
    addMetaObject<WushenSlash>();
    addMetaObject<KuangfengCard>();
    addMetaObject<DawuCard>();
    addMetaObject<WuqianCard>();
}

ADD_PACKAGE(God)
