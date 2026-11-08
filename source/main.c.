#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FLOOR 200
#define TEAM_MAX 3
#define ITEM_COUNT 12

typedef struct { const char *name; int heal; int capture; int boost; } Item;
typedef struct { const char *name; int hp; int maxhp; int atk; int level; } Mon;

static const char *species[151] = {
"Bulbizarre","Herbizarre","Florizarre","Salamèche","Reptincel","Dracaufeu",
"Carapuce","Carabaffe","Tortank","Chenipan","Chrysacier","Papilusion",
"Aspicot","Coconfort","Dardargnan","Roucool","Roucoups","Roucarnage",
"Rattata","Rattatac","Piafabec","Rapasdepic","Abo","Arbok","Pikachu",
"Raichu","Sabelette","Sablaireau","Nidoran F","Nidorina","Nidoqueen",
"Nidoran M","Nidorino","Nidoking","Mélofée","Mélodelfe","Goupix","Feunard",
"Rondoudou","Grodoudou","Nosferapti","Nosferalto","Mystherbe","Ortide",
"Rafflesia","Paras","Parasect","Mimitoss","Aéromite","Taupiqueur",
"Triopikeur","Miaouss","Persian","Psykokwak","Akwakwak","Férosinge",
"Colossinge","Caninos","Arcanin","Ptitard","Têtarte","Tartard","Abra",
"Kadabra","Alakazam","Machoc","Machopeur","Mackogneur","Chétiflor",
"Boustiflor","Empiflor","Tentacool","Tentacruel","Racaillou","Gravalanch",
"Grolem","Ponyta","Galopa","Ramoloss","Flagadoss","Magnéti","Magnéton",
"Canarticho","Doduo","Dodrio","Otaria","Lamantine","Tadmorv","Grotadmorv",
"Kokiyas","Crustabri","Fantominus","Spectrum","Ectoplasma","Onix","Soporifik",
"Hypnomade","Krabby","Krabboss","Voltorbe","Électrode","Noeunoeuf",
"Noadkoko","Osselait","Ossatueur","Kicklee","Tygnon","Excelangue","Smogo",
"Smogogo","Rhinocorne","Rhinoféros","Leveinard","Saquedeneu","Kangourex",
"Hypotrempe","Hypocéan","Poissirène","Poissoroy","Stari","Staross",
"M. Mime","Insécateur","Lippoutou","Élektek","Magmar","Scarabrute",
"Tauros","Magicarpe","Léviator","Lokhlass","Métamorph","Évoli","Aquali",
"Voltali","Pyroli","Porygon","Amonita","Amonistar","Kabuto","Kabutops",
"Ptéra","Ronflex","Artikodin","Électhor","Sulfura","Minidraco","Draco",
"Dracolosse","Mewtwo","Mew"
};

static Item items[ITEM_COUNT] = {
 {"Potion",20,0,0},{"Super Potion",50,0,0},{"Hyper Potion",120,0,0},
 {"Rappel",0,0,0},{"Poké Ball",0,1,0},{"Super Ball",0,2,0},
 {"Hyper Ball",0,3,0},{"Pierre puissance",0,0,2},{"Garde solide",0,0,1},
 {"Baie soin",15,0,0},{"Bonbon expérience",0,0,3},{"Orbe chance",0,0,1}
};

static Mon team[TEAM_MAX];
static int teamCount=1, floorNo=1, gold=0, potions=2, balls=3, selected=0;
static int enemyHp, enemyMax, enemyAtk, enemyLevel, isBoss, isMajor;
static int running=1, inBattle=0, menu=0, wins=0;
static int heldBoost=0, expCandy=0;

static void waitA(void) { while(1){ scanKeys(); if(keysDown() & (KEY_A|KEY_B)) break; swiWaitForVBlank(); } }
static void drawTitle(void) {
 consoleClear();
 iprintf("\x1b[2;3HPokeRogue DS - prototype 0.4");
 iprintf("\x1b[5;2HRun roguelite - 200 etages");
 iprintf("\x1b[8;2HEquipe: %d/%d",teamCount,TEAM_MAX);
 iprintf("\x1b[10;2HEtage: %d / %d",floorNo,MAX_FLOOR);
 iprintf("\x1b[12;2HArgent: %d G",gold);
 iprintf("\x1b[15;2HA: Continuer");
 iprintf("\x1b[17;2HSTART: Equipe / objets");
}
static void startEnemy(void) {
 isMajor = (floorNo % 20 == 0);
 isBoss = (!isMajor && floorNo % 5 == 0);
 enemyLevel = 3 + floorNo * 2;
 enemyMax = 18 + floorNo * 5 + (isBoss?25:0) + (isMajor?80:0);
 enemyHp = enemyMax;
 enemyAtk = 3 + floorNo/3 + (isBoss?3:0) + (isMajor?8:0);
 inBattle=1;
}
static void drawBattle(void) {
 consoleClear();
 iprintf("\x1b[1;2HETAGE %d / %d",floorNo,MAX_FLOOR);
 if(isMajor) iprintf("\x1b[3;2HBOSS MAJEUR !");
 else if(isBoss) iprintf("\x1b[3;2HBOSS !");
 else iprintf("\x1b[3;2HRencontre sauvage");
 iprintf("\x1b[5;2H%s niv.%d",species[(floorNo*7+floorNo/3)%151],enemyLevel);
 iprintf("\x1b[6;2HPV ennemi: %d/%d",enemyHp,enemyMax);
 iprintf("\x1b[9;2H%s niv.%d",team[0].name,team[0].level);
 iprintf("\x1b[10;2HPV: %d/%d",team[0].hp,team[0].maxhp);
 iprintf("\x1b[13;2HA: Attaque   X: Capacite");
 iprintf("\x1b[15;2HY: Capturer  B: Potion");
 iprintf("\x1b[17;2HL: Fuir");
}
static void reward(void) {
 consoleClear();
 gold += 5 + floorNo*2 + (isBoss?20:0) + (isMajor?75:0);
 wins++;
 iprintf("\x1b[3;3HCombat gagne !");
 iprintf("\x1b[6;3HRecompense: %d G",5+floorNo*2);
 if(isBoss) iprintf("\x1b[8;3HBoss vaincu : bonus !");
 if(isMajor) iprintf("\x1b[9;3HBoss majeur vaincu !");
 iprintf("\x1b[12;3HObjets: A=Potion  X=Pokeball");
 iprintf("\x1b[15;3HA pour etage suivant");
 waitA();
 if(floorNo%3==0) potions++;
 if(floorNo%5==0) balls++;
 if(floorNo%10==0) heldBoost++;
 floorNo++;
 if(floorNo>MAX_FLOOR) { consoleClear(); iprintf("\x1b[8;4HRun terminee !"); iprintf("\x1b[10;4HEtages: 200"); waitA(); running=0; }
 inBattle=0;
}
static void enemyTurn(void) {
 team[0].hp -= enemyAtk;
 if(team[0].hp<0) team[0].hp=0;
}
static void attack(int power) {
 int damage=team[0].atk+power+heldBoost;
 enemyHp-=damage;
 if(enemyHp<=0){enemyHp=0; reward(); return;}
 enemyTurn();
 if(team[0].hp<=0){consoleClear(); iprintf("\x1b[8;4HEquipe vaincue..."); waitA(); running=0;}
}
static void battleInput(void) {
 scanKeys(); u32 k=keysDown();
 if(k&KEY_A) attack(2);
 if(k&KEY_X) attack(7);
 if(k&KEY_B) {
   if(potions>0 && team[0].hp<team[0].maxhp) {
     potions--; team[0].hp+=35;
     if(team[0].hp>team[0].maxhp) team[0].hp=team[0].maxhp;
     enemyTurn();
   }
 }
 if(k&KEY_Y) {
   if(balls>0 && !isMajor) {
     balls--;
     int chance=(enemyHp*100/enemyMax);
     if(chance<55){teamCount=teamCount<TEAM_MAX?teamCount+1:teamCount; reward(); return;}
     enemyTurn();
   }
 }
 if(k&KEY_L) { if(!isBoss && !isMajor){inBattle=0; floorNo++;} else enemyTurn(); }
}
static void initRun(void) {
 team[0].name="Bulbizarre"; team[0].hp=40; team[0].maxhp=40; team[0].atk=6; team[0].level=5;
}
int main(void) {
 consoleDemoInit();
 initRun();
 while(running) {
   if(!inBattle) {
     drawTitle();
     scanKeys();
     if(keysDown()&KEY_START) {
       consoleClear();
       iprintf("EQUIPE / SAC");
       iprintf("\n%s niv.%d  PV %d/%d",team[0].name,team[0].level,team[0].hp,team[0].maxhp);
       iprintf("\nPotions: %d  Balls: %d",potions,balls);
       iprintf("\nBoosts: %d  Bonbons XP: %d",heldBoost,expCandy);
       iprintf("\nA pour revenir");
       waitA();
     } else if(keysDown()&KEY_A) startEnemy();
   } else {
     drawBattle();
     battleInput();
   }
   swiWaitForVBlank();
 }
 consoleClear();
 iprintf("\x1b[8;3HMerci d'avoir joue !");
 while(1) swiWaitForVBlank();
 return 0;
}
