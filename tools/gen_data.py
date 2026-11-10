#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Génère source/data.c et include/data.h
Données : 151 espèces (stats de base modernes à 6 valeurs, types actuels dont Fée),
évolutions, attaques, table des types, biomes.
Usage : python3 tools/gen_data.py   (depuis la racine du projet)
"""
import os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

TYPES = ["Normal", "Feu", "Eau", "Électrik", "Plante", "Glace", "Combat", "Poison", "Sol",
         "Vol", "Psy", "Insecte", "Roche", "Spectre", "Dragon", "Ténèbres", "Acier", "Fée"]
TC = {n: i for i, n in enumerate(["NOR", "FEU", "EAU", "ELE", "PLA", "GLA", "COM", "POI", "SOL",
                                  "VOL", "PSY", "INS", "ROC", "SPE", "DRA", "TEN", "ACI", "FEE"])}
TYPE_RGB = [(168, 168, 120), (240, 128, 48), (104, 144, 240), (248, 208, 48), (120, 200, 80),
            (152, 216, 216), (192, 48, 40), (160, 64, 160), (224, 192, 104), (168, 144, 240),
            (248, 88, 136), (168, 184, 32), (184, 160, 56), (112, 88, 152), (112, 56, 248),
            (112, 88, 72), (184, 184, 208), (238, 153, 172)]

# ---------------------------------------------------------------- type chart
SE = {  # attaquant: (super efficace, peu efficace, immunisé)
    "NOR": ("", "ROC ACI", "SPE"),
    "FEU": ("PLA GLA INS ACI", "FEU EAU ROC DRA", ""),
    "EAU": ("FEU SOL ROC", "EAU PLA DRA", ""),
    "ELE": ("EAU VOL", "ELE PLA DRA", "SOL"),
    "PLA": ("EAU SOL ROC", "FEU PLA POI VOL INS DRA ACI", ""),
    "GLA": ("PLA SOL VOL DRA", "FEU EAU GLA ACI", ""),
    "COM": ("NOR GLA ROC TEN ACI", "POI VOL PSY INS FEE", "SPE"),
    "POI": ("PLA FEE", "POI SOL ROC SPE", "ACI"),
    "SOL": ("FEU ELE POI ROC ACI", "PLA INS", "VOL"),
    "VOL": ("PLA COM INS", "ELE ROC ACI", ""),
    "PSY": ("COM POI", "PSY ACI", "TEN"),
    "INS": ("PLA PSY TEN", "FEU COM POI VOL SPE ACI FEE", ""),
    "ROC": ("FEU GLA VOL INS", "COM SOL ACI", ""),
    "SPE": ("PSY SPE", "TEN", "NOR"),
    "DRA": ("DRA", "ACI", "FEE"),
    "TEN": ("PSY SPE", "COM TEN FEE", ""),
    "ACI": ("GLA ROC FEE", "FEU EAU ELE ACI", ""),
    "FEE": ("COM DRA TEN", "FEU POI ACI", ""),
}
chart = [[4] * 18 for _ in range(18)]   # 4 = x1, 8 = x2, 2 = x0.5, 0 = immunité
for a, (s, n, z) in SE.items():
    for t in s.split():
        chart[TC[a]][TC[t]] = 8
    for t in n.split():
        chart[TC[a]][TC[t]] = 2
    for t in z.split():
        chart[TC[a]][TC[t]] = 0

# ---------------------------------------------------------------- espèces
# nom, type1, type2, PV, Atq, Déf, AtqSpé, DéfSpé, Vit
S = """
Bulbizarre PLA POI 45 49 49 65 65 45
Herbizarre PLA POI 60 62 63 80 80 60
Florizarre PLA POI 80 82 83 100 100 80
Salamèche FEU - 39 52 43 60 50 65
Reptincel FEU - 58 64 58 80 65 80
Dracaufeu FEU VOL 78 84 78 109 85 100
Carapuce EAU - 44 48 65 50 64 43
Carabaffe EAU - 59 63 80 65 80 58
Tortank EAU - 79 83 100 85 105 78
Chenipan INS - 45 30 35 20 20 45
Chrysacier INS - 50 20 55 25 25 30
Papilusion INS VOL 60 45 50 90 80 70
Aspicot INS POI 40 35 30 20 20 50
Coconfort INS POI 45 25 50 25 25 35
Dardargnan INS POI 65 90 40 45 80 75
Roucool NOR VOL 40 45 40 35 35 56
Roucoups NOR VOL 63 60 55 50 50 71
Roucarnage NOR VOL 83 80 75 70 70 101
Rattata NOR - 30 56 35 25 35 72
Rattatac NOR - 55 81 60 50 70 97
Piafabec NOR VOL 40 60 30 31 31 70
Rapasdepic NOR VOL 65 90 65 61 61 100
Abo POI - 35 60 44 40 54 55
Arbok POI - 60 95 69 65 79 80
Pikachu ELE - 35 55 40 50 50 90
Raichu ELE - 60 90 55 90 80 110
Sabelette SOL - 50 75 85 20 30 40
Sablaireau SOL - 75 100 110 45 55 65
Nidoran♀ POI - 55 47 52 40 40 41
Nidorina POI - 70 62 67 55 55 56
Nidoqueen POI SOL 90 92 87 75 85 76
Nidoran♂ POI - 46 57 40 40 40 50
Nidorino POI - 61 72 57 55 55 65
Nidoking POI SOL 81 102 77 85 75 85
Mélofée FEE - 70 45 48 60 65 35
Mélodelfe FEE - 95 70 73 95 90 60
Goupix FEU - 38 41 40 50 65 65
Feunard FEU - 73 76 75 81 100 100
Rondoudou NOR FEE 115 45 20 45 25 20
Grodoudou NOR FEE 140 70 45 85 50 45
Nosferapti POI VOL 40 45 35 30 40 55
Nosferalto POI VOL 75 80 70 65 75 90
Mystherbe PLA POI 45 50 55 75 65 30
Ortide PLA POI 60 65 70 85 75 40
Rafflesia PLA POI 75 80 85 110 90 50
Paras INS PLA 35 70 55 45 55 25
Parasect INS PLA 60 95 80 60 80 30
Mimitoss INS POI 60 55 50 40 55 45
Aéromite INS POI 70 65 60 90 75 90
Taupiqueur SOL - 10 55 25 35 45 95
Triopikeur SOL - 35 100 50 50 70 120
Miaouss NOR - 40 45 35 40 40 90
Persian NOR - 65 70 60 65 65 115
Psykokwak EAU - 50 52 48 65 50 55
Akwakwak EAU - 80 82 78 95 80 85
Férosinge COM - 40 80 35 35 45 70
Colossinge COM - 65 105 60 60 70 95
Caninos FEU - 55 70 45 70 50 60
Arcanin FEU - 90 110 80 100 80 95
Ptitard EAU - 40 50 40 40 40 90
Têtarte EAU - 65 65 65 50 50 90
Tartard EAU COM 90 95 95 70 90 70
Abra PSY - 25 20 15 105 55 90
Kadabra PSY - 40 35 30 120 70 105
Alakazam PSY - 55 50 45 135 95 120
Machoc COM - 70 80 50 35 35 35
Machopeur COM - 80 100 70 50 60 45
Mackogneur COM - 90 130 80 65 85 55
Chétiflor PLA POI 50 75 35 70 30 40
Boustiflor PLA POI 65 90 50 85 45 55
Empiflor PLA POI 80 105 65 100 70 70
Tentacool EAU POI 40 40 35 50 100 70
Tentacruel EAU POI 80 70 65 80 120 100
Racaillou ROC SOL 40 80 100 30 30 20
Gravalanch ROC SOL 55 95 115 45 45 35
Grolem ROC SOL 80 120 130 55 65 45
Ponyta FEU - 50 85 55 65 65 90
Galopa FEU - 65 100 70 80 80 105
Ramoloss EAU PSY 90 65 65 40 40 15
Flagadoss EAU PSY 95 75 110 100 80 30
Magnéti ELE ACI 25 35 70 95 55 45
Magnéton ELE ACI 50 60 95 120 70 70
Canarticho NOR VOL 52 90 55 58 62 60
Doduo NOR VOL 35 85 45 35 35 75
Dodrio NOR VOL 60 110 70 60 60 100
Otaria EAU - 65 45 55 45 70 45
Lamantine EAU GLA 90 70 80 70 95 70
Tadmorv POI - 80 80 50 40 50 25
Grotadmorv POI - 105 105 75 65 100 50
Kokiyas EAU - 30 65 100 45 25 40
Crustabri EAU GLA 50 95 180 85 45 70
Fantominus SPE POI 30 35 30 100 35 80
Spectrum SPE POI 45 50 45 115 55 95
Ectoplasma SPE POI 60 65 60 130 75 110
Onix ROC SOL 35 45 160 30 45 70
Soporifik PSY - 60 48 45 43 90 42
Hypnomade PSY - 85 73 70 73 115 67
Krabby EAU - 30 105 90 25 25 50
Krabboss EAU - 55 130 115 50 50 75
Voltorbe ELE - 40 30 50 55 55 100
Électrode ELE - 60 50 70 80 80 150
Noeunoeuf PLA PSY 60 40 80 60 45 40
Noadkoko PLA PSY 95 95 85 125 75 55
Osselait SOL - 50 50 95 40 50 35
Ossatueur SOL - 60 80 110 50 80 45
Kicklee COM - 50 120 53 35 110 87
Tygnon COM - 50 105 79 35 110 76
Excelangue NOR - 90 55 75 60 75 30
Smogo POI - 40 65 95 60 45 35
Smogogo POI - 65 90 120 85 70 60
Rhinocorne SOL ROC 80 85 95 30 30 25
Rhinoféros SOL ROC 105 130 120 45 45 40
Leveinard NOR - 250 5 5 35 105 50
Saquedeneu PLA - 65 55 115 100 40 60
Kangourex NOR - 105 95 80 40 80 90
Hypotrempe EAU - 30 40 70 70 25 60
Hypocéan EAU - 55 65 95 95 45 85
Poissirène EAU - 45 67 60 35 50 63
Poissoroy EAU - 80 92 65 65 80 68
Stari EAU - 30 45 55 70 55 85
Staross EAU PSY 60 75 85 100 85 115
M. Mime PSY FEE 40 45 65 100 120 90
Insécateur INS VOL 70 110 80 55 80 105
Lippoutou GLA PSY 65 50 35 115 95 95
Élektek ELE - 65 83 57 95 85 105
Magmar FEU - 65 95 57 100 85 93
Scarabrute INS - 65 125 100 55 70 85
Tauros NOR - 75 100 95 40 70 110
Magicarpe EAU - 20 10 55 15 20 80
Léviator EAU VOL 95 125 79 60 100 81
Lokhlass EAU GLA 130 85 80 85 95 60
Métamorph NOR - 48 48 48 48 48 48
Évoli NOR - 55 55 50 45 65 55
Aquali EAU - 130 65 60 110 95 65
Voltali ELE - 65 65 60 110 95 130
Pyroli FEU - 65 130 60 95 110 65
Porygon NOR - 65 60 70 85 75 40
Amonita ROC EAU 35 40 100 90 55 35
Amonistar ROC EAU 70 60 125 115 70 55
Kabuto ROC EAU 30 80 90 55 45 55
Kabutops ROC EAU 60 115 105 65 70 80
Ptéra ROC VOL 80 105 65 60 75 130
Ronflex NOR - 160 110 65 65 110 30
Artikodin GLA VOL 90 85 100 95 125 85
Électhor ELE VOL 90 90 85 125 90 100
Sulfura FEU VOL 90 100 90 125 85 90
Minidraco DRA - 41 64 45 50 50 50
Draco DRA - 61 84 65 70 70 70
Dracolosse DRA VOL 91 134 95 100 100 80
Mewtwo PSY - 106 110 90 154 90 130
Mew PSY - 100 100 100 100 100 100
"""
SPECIES = []
for line in S.strip().splitlines():
    p = line.split()
    # nom peut contenir un espace ("M. Mime")
    if p[0] == "M.":
        p = ["M. Mime"] + p[2:]
    name, t1, t2 = p[0], p[1], p[2]
    st = [int(x) for x in p[3:9]]
    SPECIES.append((name, TC[t1], TC[t2] if t2 != "-" else 255, st))
assert len(SPECIES) == 151, len(SPECIES)

# évolutions : dex -> (dex suivant, niveau). Les pierres/échanges sont convertis en niveaux.
EVO = {1: (2, 16), 2: (3, 32), 4: (5, 16), 5: (6, 36), 7: (8, 16), 8: (9, 36), 10: (11, 7), 11: (12, 10),
       13: (14, 7), 14: (15, 10), 16: (17, 18), 17: (18, 36), 19: (20, 20), 21: (22, 20), 23: (24, 22),
       25: (26, 30), 27: (28, 22), 29: (30, 16), 30: (31, 36), 32: (33, 16), 33: (34, 36), 35: (36, 32),
       37: (38, 32), 39: (40, 32), 41: (42, 22), 43: (44, 21), 44: (45, 36), 46: (47, 24), 48: (49, 31),
       50: (51, 26), 52: (53, 28), 54: (55, 33), 56: (57, 28), 58: (59, 32), 60: (61, 25), 61: (62, 38),
       63: (64, 16), 64: (65, 36), 66: (67, 28), 67: (68, 40), 69: (70, 21), 70: (71, 36), 72: (73, 30),
       74: (75, 25), 75: (76, 40), 77: (78, 40), 79: (80, 37), 81: (82, 30), 84: (85, 31), 86: (87, 34),
       88: (89, 38), 90: (91, 32), 92: (93, 25), 93: (94, 40), 96: (97, 26), 98: (99, 28), 100: (101, 30),
       102: (103, 30), 104: (105, 28), 109: (110, 35), 111: (112, 42), 116: (117, 32), 118: (119, 33),
       120: (121, 33), 129: (130, 20), 133: (134, 30), 138: (139, 40), 140: (141, 40), 147: (148, 30),
       148: (149, 55)}
LEGEND = {144, 145, 146, 150, 151}

NATURES_FR = ["Hardi", "Solo", "Brave", "Rigide", "Mauvais", "Assuré", "Docile", "Relax", "Malin", "Lâche",
              "Timide", "Pressé", "Sérieux", "Jovial", "Naïf", "Modeste", "Doux", "Discret", "Pudique",
              "Foufou", "Calme", "Gentil", "Malpoli", "Prudent", "Bizarre"]

# archétype de sprite (une lettre par espèce, dans l'ordre du Pokédex)
# on construit proprement par table explicite pour éviter les erreurs de découpage
ARCH_LIST = {
    1: "Q", 2: "Q", 3: "Q", 4: "B", 5: "B", 6: "D", 7: "B", 8: "B", 9: "B", 10: "S", 11: "O", 12: "I", 13: "S",
    14: "O", 15: "I", 16: "W", 17: "W", 18: "W", 19: "Q", 20: "Q", 21: "W", 22: "W", 23: "S", 24: "S", 25: "B",
    26: "B", 27: "B", 28: "B", 29: "Q", 30: "Q", 31: "B", 32: "Q", 33: "Q", 34: "B", 35: "B", 36: "B", 37: "Q",
    38: "Q", 39: "O", 40: "O", 41: "W", 42: "W", 43: "P", 44: "P", 45: "P", 46: "I", 47: "I", 48: "O", 49: "I",
    50: "O", 51: "O", 52: "Q", 53: "Q", 54: "B", 55: "B", 56: "B", 57: "B", 58: "Q", 59: "Q", 60: "O", 61: "O",
    62: "B", 63: "B", 64: "H", 65: "H", 66: "H", 67: "H", 68: "H", 69: "P", 70: "P", 71: "P", 72: "J", 73: "J",
    74: "R", 75: "R", 76: "R", 77: "Q", 78: "Q", 79: "Q", 80: "B", 81: "L", 82: "L", 83: "W", 84: "W", 85: "W",
    86: "F", 87: "F", 88: "O", 89: "O", 90: "E", 91: "E", 92: "G", 93: "G", 94: "B", 95: "S", 96: "B", 97: "B",
    98: "I", 99: "I", 100: "L", 101: "L", 102: "L", 103: "P", 104: "B", 105: "B", 106: "H", 107: "H", 108: "B",
    109: "L", 110: "L", 111: "Q", 112: "B", 113: "B", 114: "O", 115: "B", 116: "F", 117: "F", 118: "F", 119: "F",
    120: "T", 121: "T", 122: "H", 123: "I", 124: "H", 125: "H", 126: "H", 127: "I", 128: "Q", 129: "F", 130: "S",
    131: "Q", 132: "O", 133: "Q", 134: "Q", 135: "Q", 136: "Q", 137: "B", 138: "E", 139: "E", 140: "E", 141: "B",
    142: "W", 143: "O", 144: "W", 145: "W", 146: "W", 147: "S", 148: "S", 149: "D", 150: "H", 151: "B"}
assert len(ARCH_LIST) == 151

# couleurs de base particulières (R,G,B) pour les icônes les plus reconnaissables
COLOR_OVERRIDE = {
    1: (104, 184, 152), 2: (96, 168, 136), 3: (88, 152, 120), 4: (240, 128, 48), 7: (96, 160, 224),
    8: (80, 140, 210), 9: (72, 120, 190), 12: (150, 120, 200), 15: (240, 208, 64), 16: (200, 160, 112),
    17: (200, 150, 100), 18: (210, 160, 100), 19: (150, 110, 170), 20: (170, 120, 90), 21: (200, 150, 100),
    23: (160, 90, 190), 25: (248, 208, 48), 26: (240, 160, 40), 29: (110, 150, 220), 32: (180, 90, 170),
    35: (248, 190, 200), 37: (200, 100, 60), 39: (248, 170, 200), 41: (96, 110, 200), 43: (80, 110, 190),
    52: (232, 208, 150), 54: (240, 220, 90), 56: (230, 220, 200), 58: (240, 150, 60), 60: (90, 120, 220),
    63: (225, 195, 90), 72: (110, 150, 230), 74: (160, 150, 130), 77: (250, 240, 200), 79: (240, 150, 170),
    81: (190, 200, 210), 92: (110, 70, 150), 93: (110, 70, 150), 94: (96, 64, 140), 95: (150, 150, 150),
    100: (230, 80, 70), 101: (230, 80, 70), 102: (240, 190, 190), 104: (170, 130, 90), 109: (170, 130, 190),
    113: (248, 190, 200), 116: (110, 160, 230), 120: (170, 120, 70), 121: (150, 100, 190), 129: (235, 90, 60),
    130: (80, 120, 210), 131: (90, 140, 210), 132: (200, 170, 230), 133: (190, 140, 90), 143: (70, 120, 120),
    144: (150, 210, 240), 145: (250, 220, 60), 146: (250, 160, 40), 147: (110, 160, 230), 148: (110, 160, 230),
    149: (240, 170, 80), 150: (200, 190, 220), 151: (248, 180, 200),
}

# ---------------------------------------------------------------- attaques
# (id, nom, type, cat 0=phys 1=spé 2=statut, puissance, précision(0=ne rate jamais), PP, priorité, effet, chance, p1, p2, p3)
EFFECTS = ["NONE", "STATUS", "FLINCH", "STAT_FOE", "STAT_SELF", "DRAIN", "RECOIL", "MULTI", "HEAL", "REST",
           "FIXED", "HICRIT", "SUICIDE", "NOTHING", "LEVELDMG"]
STATUS = {"BRN": 1, "PAR": 2, "PSN": 3, "TOX": 4, "SLP": 5, "FRZ": 6, "CNF": 7}
STAT = {"ATK": 0, "DEF": 1, "SPA": 2, "SPD": 3, "SPE": 4}
MOVES = []


def mv(ident, name, typ, cat, pw, acc, pp, prio=0, eff="NONE", ch=0, p1=0, p2=0, p3=255):
    MOVES.append((ident, name, TC[typ], cat, pw, acc, pp, prio, EFFECTS.index(eff), ch, p1, p2, p3))


P, Sp, St = 0, 1, 2
mv("LUTTE", "Lutte", "NOR", P, 50, 0, 1, 0, "RECOIL", 0, 4)
mv("TREMPETTE", "Trempette", "NOR", St, 0, 0, 40, 0, "NOTHING")
mv("CHARGE", "Charge", "NOR", P, 40, 100, 35)
mv("GRIFFE", "Griffe", "NOR", P, 40, 100, 35)
mv("ECRASFACE", "Écras'Face", "NOR", P, 40, 100, 35)
mv("VIVE_ATTAQUE", "Vive-Attaque", "NOR", P, 40, 100, 30, 1)
mv("PLAQUAGE", "Plaquage", "NOR", P, 85, 100, 15, 0, "STATUS", 30, STATUS["PAR"])
mv("FORCE", "Force", "NOR", P, 80, 100, 15)
mv("TRANCHE", "Tranche", "NOR", P, 70, 100, 20, 0, "HICRIT")
mv("DAMOCLES", "Damoclès", "NOR", P, 120, 100, 15, 0, "RECOIL", 0, 3)
mv("GIGA_IMPACT", "Giga Impact", "NOR", P, 150, 90, 5)
mv("ULTRALASER", "Ultralaser", "NOR", Sp, 150, 90, 5)
mv("TRIPLATTAQUE", "Triplattaque", "NOR", Sp, 80, 100, 10, 0, "STATUS", 20, STATUS["PAR"])
mv("COMBO_GRIFFE", "Combo-Griffe", "NOR", P, 18, 80, 15, 0, "MULTI")
mv("TORGNOLES", "Torgnoles", "NOR", P, 15, 85, 10, 0, "MULTI")
mv("DESTRUCTION", "Destruction", "NOR", P, 200, 100, 5, 0, "SUICIDE")
mv("EXPLOSION", "Explosion", "NOR", P, 250, 100, 5, 0, "SUICIDE")
mv("RUGISSEMENT", "Rugissement", "NOR", St, 0, 100, 40, 0, "STAT_FOE", 100, STAT["ATK"], -1)
mv("MIMI_QUEUE", "Mimi-Queue", "NOR", St, 0, 100, 30, 0, "STAT_FOE", 100, STAT["DEF"], -1)
mv("ULTRASON", "Ultrason", "NOR", St, 0, 55, 20, 0, "STATUS", 100, STATUS["CNF"])
mv("BERCEUSE", "Berceuse", "NOR", St, 0, 55, 15, 0, "STATUS", 100, STATUS["SLP"])
mv("CROISSANCE", "Croissance", "NOR", St, 0, 0, 20, 0, "STAT_SELF", 100, STAT["ATK"], 1, STAT["SPA"])
mv("ARMURE", "Armure", "NOR", St, 0, 0, 30, 0, "STAT_SELF", 100, STAT["DEF"], 1)
mv("DANSE_LAMES", "Danse-Lames", "NOR", St, 0, 0, 20, 0, "STAT_SELF", 100, STAT["ATK"], 2)
mv("SOIN", "Soin", "NOR", St, 0, 0, 10, 0, "HEAL")
mv("FEU_FOLLET", "Feu Follet", "FEU", St, 0, 85, 15, 0, "STATUS", 100, STATUS["BRN"])
mv("FLAMMECHE", "Flammèche", "FEU", Sp, 40, 100, 25, 0, "STATUS", 10, STATUS["BRN"])
mv("POING_FEU", "Poing de Feu", "FEU", P, 75, 100, 15, 0, "STATUS", 10, STATUS["BRN"])
mv("LANCE_FLAMMES", "Lance-Flammes", "FEU", Sp, 90, 100, 15, 0, "STATUS", 10, STATUS["BRN"])
mv("DEFLAGRATION", "Déflagration", "FEU", Sp, 110, 85, 5, 0, "STATUS", 10, STATUS["BRN"])
mv("BOUTEFEU", "Boutefeu", "FEU", P, 120, 100, 15, 0, "RECOIL", 0, 3)
mv("PISTOLET_O", "Pistolet à O", "EAU", Sp, 40, 100, 25)
mv("AQUA_JET", "Aqua-Jet", "EAU", P, 40, 100, 20, 1)
mv("BULLES_DO", "Bulles d'O", "EAU", Sp, 65, 100, 20, 0, "STAT_FOE", 10, STAT["SPE"], -1)
mv("CASCADE", "Cascade", "EAU", P, 80, 100, 15, 0, "FLINCH", 20)
mv("SURF", "Surf", "EAU", Sp, 90, 100, 15)
mv("PINCE_MASSE", "Pince-Masse", "EAU", P, 100, 90, 10, 0, "HICRIT")
mv("HYDROCANON", "Hydrocanon", "EAU", Sp, 110, 80, 5)
mv("ECLAIR", "Éclair", "ELE", Sp, 40, 100, 30, 0, "STATUS", 10, STATUS["PAR"])
mv("ELECTACLE", "Étincelle", "ELE", P, 65, 100, 20, 0, "STATUS", 30, STATUS["PAR"])
mv("POING_ECLAIR", "Poing-Éclair", "ELE", P, 75, 100, 15, 0, "STATUS", 10, STATUS["PAR"])
mv("TONNERRE", "Tonnerre", "ELE", Sp, 90, 100, 15, 0, "STATUS", 10, STATUS["PAR"])
mv("FATAL_FOUDRE", "Fatal-Foudre", "ELE", Sp, 110, 70, 10, 0, "STATUS", 30, STATUS["PAR"])
mv("VOLTAGE_FOU", "Éclair Fou", "ELE", P, 90, 100, 15, 0, "RECOIL", 0, 4)
mv("CAGE_ECLAIR", "Cage-Éclair", "ELE", St, 0, 90, 20, 0, "STATUS", 100, STATUS["PAR"])
mv("FOUET_LIANES", "Fouet Lianes", "PLA", P, 45, 100, 25)
mv("TRANCH_HERBE", "Tranch'Herbe", "PLA", P, 55, 95, 25, 0, "HICRIT")
mv("MEGASANGSUE", "Méga-Sangsue", "PLA", Sp, 40, 100, 15, 0, "DRAIN")
mv("GIGA_SANGSUE", "Giga-Sangsue", "PLA", Sp, 75, 100, 10, 0, "DRAIN")
mv("LAME_FEUILLE", "Lame-Feuille", "PLA", P, 90, 100, 15, 0, "HICRIT")
mv("ECO_SPHERE", "Éco-Sphère", "PLA", Sp, 90, 100, 10)
mv("LANCE_SOLEIL", "Lance-Soleil", "PLA", Sp, 120, 100, 10)
mv("PARA_SPORE", "Para-Spore", "PLA", St, 0, 75, 30, 0, "STATUS", 100, STATUS["PAR"])
mv("POUDRE_DODO", "Poudre Dodo", "PLA", St, 0, 75, 15, 0, "STATUS", 100, STATUS["SLP"])
mv("POUDRE_TOXIK", "Poudre Toxik", "PLA", St, 0, 75, 35, 0, "STATUS", 100, STATUS["PSN"])
mv("SPORE", "Spore", "PLA", St, 0, 100, 15, 0, "STATUS", 100, STATUS["SLP"])
mv("POUDREUSE", "Poudreuse", "GLA", Sp, 40, 100, 25, 0, "STATUS", 10, STATUS["FRZ"])
mv("ECLATS_GLACE", "Éclats Glace", "GLA", P, 40, 100, 30, 1)
mv("POING_GLACE", "Poing Glace", "GLA", P, 75, 100, 15, 0, "STATUS", 10, STATUS["FRZ"])
mv("LASER_GLACE", "Laser Glace", "GLA", Sp, 90, 100, 10, 0, "STATUS", 10, STATUS["FRZ"])
mv("STALACTITE", "Stalactite", "GLA", P, 85, 90, 10, 0, "FLINCH", 30)
mv("BLIZZARD", "Blizzard", "GLA", Sp, 110, 70, 5, 0, "STATUS", 10, STATUS["FRZ"])
mv("POING_KARATE", "Poing Karaté", "COM", P, 50, 100, 25, 0, "HICRIT")
mv("MACH_PUNCH", "Mach Punch", "COM", P, 40, 100, 30, 1)
mv("BALAYAGE", "Balayage", "COM", P, 65, 100, 20, 0, "STAT_FOE", 100, STAT["SPE"], -1)
mv("CASSE_BRIQUE", "Casse-Brique", "COM", P, 75, 100, 15)
mv("AURASPHERE", "Aurasphère", "COM", Sp, 80, 0, 20)
mv("CLOSE_COMBAT", "Close Combat", "COM", P, 120, 100, 5, 0, "STAT_SELF", 100, STAT["DEF"], -1, STAT["SPD"])
mv("EXPLOFORCE", "Exploforce", "COM", Sp, 120, 70, 5)
mv("ACIDE", "Acide", "POI", Sp, 40, 100, 30, 0, "STAT_FOE", 10, STAT["SPD"], -1)
mv("DETRITUS", "Détritus", "POI", Sp, 65, 100, 20, 0, "STATUS", 30, STATUS["PSN"])
mv("DIRECT_TOXIK", "Direct Toxik", "POI", P, 80, 100, 20, 0, "STATUS", 30, STATUS["PSN"])
mv("BOMB_BEURK", "Bomb Beurk", "POI", Sp, 90, 100, 10, 0, "STATUS", 30, STATUS["PSN"])
mv("DETRICANON", "Détricanon", "POI", P, 120, 80, 5, 0, "STATUS", 30, STATUS["PSN"])
mv("TOXIK", "Toxik", "POI", St, 0, 90, 10, 0, "STATUS", 100, STATUS["TOX"])
mv("OSMERANG", "Osmerang", "SOL", P, 50, 90, 10, 0, "MULTI", 0, 2)
mv("TIR_BOUE", "Tir de Boue", "SOL", Sp, 55, 95, 15, 0, "STAT_FOE", 100, STAT["SPE"], -1)
mv("BOUE_BOMBE", "Boue-Bombe", "SOL", Sp, 65, 85, 10)
mv("TUNNEL", "Tunnel", "SOL", P, 80, 100, 10)
mv("TELLURIFORCE", "Telluriforce", "SOL", Sp, 90, 100, 10, 0, "STAT_FOE", 10, STAT["SPD"], -1)
mv("SEISME", "Séisme", "SOL", P, 100, 100, 10)
mv("PICPIC", "Picpic", "VOL", P, 35, 100, 35)
mv("TORNADE", "Tornade", "VOL", Sp, 40, 100, 35)
mv("CRU_AILE", "Cru-Aile", "VOL", P, 60, 100, 35)
mv("AEROPIQUE", "Aéropique", "VOL", P, 60, 0, 20)
mv("LAME_AIR", "Lame d'Air", "VOL", Sp, 75, 95, 15, 0, "FLINCH", 30)
mv("BEC_VRILLE", "Bec Vrille", "VOL", P, 80, 100, 20)
mv("RAPACE", "Rapace", "VOL", P, 120, 100, 15, 0, "RECOIL", 0, 3)
mv("OURAGAN", "Ouragan", "VOL", Sp, 110, 70, 10, 0, "STATUS", 30, STATUS["CNF"])
mv("CHOC_MENTAL", "Choc Mental", "PSY", Sp, 50, 100, 25, 0, "STATUS", 10, STATUS["CNF"])
mv("RAFALE_PSY", "Rafale Psy", "PSY", Sp, 65, 100, 20, 0, "STATUS", 10, STATUS["CNF"])
mv("PSYKOUD_BOULE", "Psykoud'Boule", "PSY", P, 80, 90, 15, 0, "FLINCH", 20)
mv("PSYKO", "Psyko", "PSY", Sp, 90, 100, 10, 0, "STAT_FOE", 10, STAT["SPD"], -1)
mv("HYPNOSE", "Hypnose", "PSY", St, 0, 60, 20, 0, "STATUS", 100, STATUS["SLP"])
mv("CALME_ESPRIT", "Plénitude", "PSY", St, 0, 0, 20, 0, "STAT_SELF", 100, STAT["SPA"], 1, STAT["SPD"])
mv("AMNESIE", "Amnésie", "PSY", St, 0, 0, 20, 0, "STAT_SELF", 100, STAT["SPD"], 2)
mv("HATE", "Hâte", "PSY", St, 0, 0, 30, 0, "STAT_SELF", 100, STAT["SPE"], 2)
mv("REPOS", "Repos", "PSY", St, 0, 0, 10, 0, "REST")
mv("PIQURE", "Piqûre", "INS", P, 60, 100, 20)
mv("DOUBLE_DARD", "Double Dard", "INS", P, 25, 100, 20, 0, "MULTI", 0, 2)
mv("BOURDON", "Bourdon", "INS", Sp, 90, 100, 10, 0, "STAT_FOE", 10, STAT["SPD"], -1)
mv("PLAIE_CROIX", "Plaie-Croix", "INS", P, 80, 100, 15)
mv("MEGACORNE", "Mégacorne", "INS", P, 120, 85, 10)
mv("SECRETION", "Sécrétion", "INS", St, 0, 95, 40, 0, "STAT_FOE", 100, STAT["SPE"], -2)
mv("JET_PIERRES", "Jet-Pierres", "ROC", P, 50, 90, 15)
mv("POUVOIR_ANTIQUE", "Pouvoir Antique", "ROC", Sp, 60, 100, 5)
mv("ECLOBEMENT", "Éboulement", "ROC", P, 75, 90, 10, 0, "FLINCH", 30)
mv("RAYON_GEMME", "Rayon Gemme", "ROC", Sp, 80, 100, 20)
mv("LAME_ROC", "Lame de Roc", "ROC", P, 100, 80, 5, 0, "HICRIT")
mv("OMBRE_PORTEE", "Ombre Portée", "SPE", P, 40, 100, 30, 1)
mv("LECHOUILLE", "Léchouille", "SPE", P, 30, 100, 30, 0, "STATUS", 30, STATUS["PAR"])
mv("POING_OMBRE", "Poing Ombre", "SPE", P, 60, 0, 20)
mv("GRIFFE_OMBRE", "Griffe Ombre", "SPE", P, 70, 100, 15, 0, "HICRIT")
mv("BALL_OMBRE", "Ball'Ombre", "SPE", Sp, 80, 100, 15, 0, "STAT_FOE", 20, STAT["SPD"], -1)
mv("TENEBRES", "Ténèbres", "SPE", Sp, 1, 100, 15, 0, "LEVELDMG")
mv("DRACO_RAGE", "Draco-Rage", "DRA", Sp, 1, 100, 10, 0, "FIXED", 0, 40)
mv("DRACO_GRIFFE", "Draco-Griffe", "DRA", P, 80, 100, 15)
mv("DRACOCHOC", "Dracochoc", "DRA", Sp, 85, 100, 10)
mv("COLERE", "Colère", "DRA", P, 120, 100, 10)
mv("DRACO_METEORE", "Draco-Météore", "DRA", Sp, 130, 90, 5, 0, "STAT_SELF", 100, STAT["SPA"], -2)
mv("DANSE_DRACO", "Danse Draco", "DRA", St, 0, 0, 20, 0, "STAT_SELF", 100, STAT["ATK"], 1, STAT["SPE"])
mv("POURSUITE", "Poursuite", "TEN", P, 40, 100, 20)
mv("ABOIEMENT", "Aboiement", "TEN", Sp, 55, 95, 15, 0, "STAT_FOE", 100, STAT["SPA"], -1)
mv("MORSURE", "Morsure", "TEN", P, 60, 100, 25, 0, "FLINCH", 30)
mv("VIBROBSCUR", "Vibrobscur", "TEN", Sp, 80, 100, 15, 0, "FLINCH", 20)
mv("TRICHERIE", "Tricherie", "TEN", P, 95, 100, 15)
mv("MACHINATION", "Machination", "TEN", St, 0, 0, 20, 0, "STAT_SELF", 100, STAT["SPA"], 2)
mv("GRIFFE_ACIER", "Griffe Acier", "ACI", P, 50, 95, 35)
mv("TETE_DE_FER", "Tête de Fer", "ACI", P, 80, 100, 15, 0, "FLINCH", 30)
mv("LUMINOCANON", "Luminocanon", "ACI", Sp, 80, 100, 10, 0, "STAT_FOE", 10, STAT["SPD"], -1)
mv("QUEUE_DE_FER", "Queue de Fer", "ACI", P, 100, 75, 15, 0, "STAT_FOE", 30, STAT["DEF"], -1)
mv("MUR_DE_FER", "Mur de Fer", "ACI", St, 0, 0, 15, 0, "STAT_SELF", 100, STAT["DEF"], 2)
mv("VENT_FEERIQUE", "Vent Féérique", "FEE", Sp, 40, 100, 30)
mv("ECLAT_MAGIQUE", "Éclat Magique", "FEE", Sp, 80, 100, 10)
mv("CALINERIE", "Câlinerie", "FEE", P, 90, 90, 10, 0, "STAT_FOE", 10, STAT["ATK"], -1)
mv("POUVOIR_LUNAIRE", "Pouvoir Lunaire", "FEE", Sp, 95, 100, 15, 0, "STAT_FOE", 30, STAT["SPA"], -1)

MID = {m[0]: i for i, m in enumerate(MOVES)}
assert len(MOVES) < 255
assert len({m[0] for m in MOVES}) == len(MOVES)

# paliers d'attaques par type : pour chaque type 4 paliers (physique, spécial)
TIERS = {
    "NOR": [("CHARGE", "ECRASFACE"), ("PLAQUAGE", "TRIPLATTAQUE"), ("FORCE", "TRIPLATTAQUE"), ("DAMOCLES", "ULTRALASER")],
    "FEU": [("POING_FEU", "FLAMMECHE"), ("POING_FEU", "FLAMMECHE"), ("POING_FEU", "LANCE_FLAMMES"), ("BOUTEFEU", "DEFLAGRATION")],
    "EAU": [("AQUA_JET", "PISTOLET_O"), ("CASCADE", "BULLES_DO"), ("PINCE_MASSE", "SURF"), ("PINCE_MASSE", "HYDROCANON")],
    "ELE": [("ELECTACLE", "ECLAIR"), ("POING_ECLAIR", "ECLAIR"), ("POING_ECLAIR", "TONNERRE"), ("VOLTAGE_FOU", "FATAL_FOUDRE")],
    "PLA": [("FOUET_LIANES", "MEGASANGSUE"), ("TRANCH_HERBE", "MEGASANGSUE"), ("LAME_FEUILLE", "GIGA_SANGSUE"), ("LAME_FEUILLE", "LANCE_SOLEIL")],
    "GLA": [("ECLATS_GLACE", "POUDREUSE"), ("POING_GLACE", "POUDREUSE"), ("STALACTITE", "LASER_GLACE"), ("STALACTITE", "BLIZZARD")],
    "COM": [("MACH_PUNCH", "AURASPHERE"), ("POING_KARATE", "AURASPHERE"), ("CASSE_BRIQUE", "AURASPHERE"), ("CLOSE_COMBAT", "EXPLOFORCE")],
    "POI": [("DIRECT_TOXIK", "ACIDE"), ("DIRECT_TOXIK", "DETRITUS"), ("DIRECT_TOXIK", "BOMB_BEURK"), ("DETRICANON", "BOMB_BEURK")],
    "SOL": [("OSMERANG", "TIR_BOUE"), ("TUNNEL", "BOUE_BOMBE"), ("SEISME", "TELLURIFORCE"), ("SEISME", "TELLURIFORCE")],
    "VOL": [("PICPIC", "TORNADE"), ("CRU_AILE", "TORNADE"), ("BEC_VRILLE", "LAME_AIR"), ("RAPACE", "OURAGAN")],
    "PSY": [("PSYKOUD_BOULE", "CHOC_MENTAL"), ("PSYKOUD_BOULE", "RAFALE_PSY"), ("PSYKOUD_BOULE", "PSYKO"), ("PSYKOUD_BOULE", "PSYKO")],
    "INS": [("DOUBLE_DARD", "PIQURE"), ("PIQURE", "BOURDON"), ("PLAIE_CROIX", "BOURDON"), ("MEGACORNE", "BOURDON")],
    "ROC": [("JET_PIERRES", "POUVOIR_ANTIQUE"), ("ECLOBEMENT", "RAYON_GEMME"), ("LAME_ROC", "RAYON_GEMME"), ("LAME_ROC", "RAYON_GEMME")],
    "SPE": [("OMBRE_PORTEE", "OMBRE_PORTEE"), ("GRIFFE_OMBRE", "BALL_OMBRE"), ("GRIFFE_OMBRE", "BALL_OMBRE"), ("GRIFFE_OMBRE", "BALL_OMBRE")],
    "DRA": [("DRACO_RAGE", "DRACO_RAGE"), ("DRACO_GRIFFE", "DRACOCHOC"), ("DRACO_GRIFFE", "DRACOCHOC"), ("COLERE", "DRACO_METEORE")],
    "TEN": [("POURSUITE", "ABOIEMENT"), ("MORSURE", "VIBROBSCUR"), ("TRICHERIE", "VIBROBSCUR"), ("TRICHERIE", "VIBROBSCUR")],
    "ACI": [("GRIFFE_ACIER", "GRIFFE_ACIER"), ("TETE_DE_FER", "LUMINOCANON"), ("TETE_DE_FER", "LUMINOCANON"), ("QUEUE_DE_FER", "LUMINOCANON")],
    "FEE": [("VENT_FEERIQUE", "VENT_FEERIQUE"), ("CALINERIE", "ECLAT_MAGIQUE"), ("CALINERIE", "POUVOIR_LUNAIRE"), ("CALINERIE", "POUVOIR_LUNAIRE")],
}
for t, tl in TIERS.items():
    for a, b in tl:
        assert a in MID and b in MID, (t, a, b)

# biomes : nom, type de fond, 4 types d'affinité
BIOMES = [
    ("Plaine", ["NOR", "VOL", "PLA", "FEE"]),
    ("Forêt", ["INS", "PLA", "POI", "NOR"]),
    ("Plage", ["EAU", "VOL", "ROC", "NOR"]),
    ("Mer", ["EAU", "GLA", "POI", "PSY"]),
    ("Montagne", ["ROC", "VOL", "COM", "SOL"]),
    ("Grotte", ["ROC", "SOL", "POI", "SPE"]),
    ("Désert", ["SOL", "FEU", "ROC", "NOR"]),
    ("Marais", ["POI", "EAU", "PLA", "INS"]),
    ("Cimetière", ["SPE", "PSY", "POI", "TEN"]),
    ("Neige", ["GLA", "EAU", "VOL", "NOR"]),
    ("Centrale", ["ELE", "ACI", "POI", "NOR"]),
    ("Volcan", ["FEU", "ROC", "SOL", "DRA"]),
    ("Dojo", ["COM", "PSY", "NOR", "TEN"]),
    ("Ruines", ["PSY", "ROC", "SPE", "FEE"]),
]
# biomes accessibles depuis chaque biome (indices)
BIOME_LINKS = [
    [1, 2, 4], [7, 0, 5], [3, 0, 6], [2, 9, 8], [5, 11, 9], [4, 8, 13], [11, 2, 5], [8, 1, 10], [13, 7, 12],
    [3, 4, 10], [13, 12, 0], [6, 4, 5], [10, 0, 13], [12, 8, 5]]

# ---------------------------------------------------------------- coût de départ (points de départ)
famMax = {}
base_of = {}
prev = {v[0]: k for k, v in EVO.items()}


def root(d):
    while d in prev:
        d = prev[d]
    return d


def final(d):
    while d in EVO:
        d = EVO[d][0]
    return d


def bst(d):
    return sum(SPECIES[d - 1][3])


costs = [0] * 152
for d in range(1, 152):
    if d in prev:
        continue
    fb = bst(final(d))
    if d == 133:
        fb = 525
    c = max(1, min(8, round((fb - 260) / 55)))
    if d == 129: c = 2
    if d in (144, 145, 146): c = 8
    if d == 150: c = 10
    if d == 151: c = 9
    if d in (147, 116): c = max(c, 5) if d == 147 else c
    costs[d] = c

# taux de capture (approximation fidèle des ordres de grandeur officiels)
def catch_rate(d):
    if d in LEGEND: return 3
    b = bst(d)
    r = 255 if b <= 300 else 190 if b <= 350 else 120 if b <= 400 else 75 if b <= 450 else 45 if b <= 500 else 25 if b <= 540 else 10
    if d in (113,): r = 30
    if d in (143, 131, 115, 142): r = 45 if d != 143 else 25
    if d in (6, 9, 3, 149, 148, 147): r = min(r, 45)
    return r


def base_exp(d):
    b = bst(d)
    if d in LEGEND: return 300
    return max(40, min(280, int(b * 0.22 + 20)))


# ---------------------------------------------------------------- sortie
def cstr(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


out = []
w = out.append
w("/* FICHIER GÉNÉRÉ PAR tools/gen_data.py — NE PAS ÉDITER À LA MAIN */")
w('#include "game.h"')
w("")
w("const char *const typeNames[TYPE_COUNT] = {" + ",".join(cstr(t) for t in TYPES) + "};")
w("const u16 typeColors[TYPE_COUNT] = {" + ",".join("RGB15(%d,%d,%d)|BIT(15)" % (r >> 3, g >> 3, b >> 3) for r, g, b in TYPE_RGB) + "};")
w("/* multiplicateurs x4 : 0, 2 (x0.5), 4, 8 (x2) */")
w("const u8 typeChart[TYPE_COUNT][TYPE_COUNT] = {")
for row in chart:
    w("  {" + ",".join(str(x) for x in row) + "},")
w("};")
w("")
w("const char *const natureNames[25] = {" + ",".join(cstr(n) for n in NATURES_FR) + "};")
w("")
w("const Species species[SPECIES_COUNT + 1] = {")
w('  {"???",0,255,{1,1,1,1,1,1},0,0,255,50,\'B\',0,0},')
for i, (name, t1, t2, st) in enumerate(SPECIES):
    d = i + 1
    evo = EVO.get(d, (0, 0))
    col = COLOR_OVERRIDE.get(d)
    colv = "RGB15(%d,%d,%d)|BIT(15)" % (col[0] >> 3, col[1] >> 3, col[2] >> 3) if col else "0"
    w("  {%s,%d,%d,{%s},%d,%d,%d,%d,'%s',%d,%s}," % (
        cstr(name), t1, t2, ",".join(str(x) for x in st), evo[0], evo[1], catch_rate(d), base_exp(d),
        ARCH_LIST[d], 1 if d in LEGEND else 0, colv))
w("};")
w("")
w("const u8 starterCost[SPECIES_COUNT + 1] = {" + ",".join(str(c) for c in costs) + "};")
w("")
w("const Move moves[MOVE_TOTAL] = {")
for m in MOVES:
    ident, name, typ, cat, pw, acc, pp, prio, eff, ch, p1, p2, p3 = m
    w("  {%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d}, /* %s */" % (cstr(name), typ, cat, pw, acc, pp, prio, eff, ch, p1, p2, p3, ident))
w("};")
w("")
w("const u8 tierMoves[TYPE_COUNT][4][2] = {")
for t in ["NOR", "FEU", "EAU", "ELE", "PLA", "GLA", "COM", "POI", "SOL", "VOL", "PSY", "INS", "ROC", "SPE", "DRA", "TEN", "ACI", "FEE"]:
    w("  {" + ",".join("{%d,%d}" % (MID[a], MID[b]) for a, b in TIERS[t]) + "},")
w("};")
w("")
w("const Biome biomes[BIOME_COUNT] = {")
for i, (name, ts) in enumerate(BIOMES):
    w("  {%s,{%s},{%s}}," % (cstr(name), ",".join(str(TC[t]) for t in ts), ",".join(str(x) for x in BIOME_LINKS[i])))
w("};")
with open(os.path.join(ROOT, "source", "data.c"), "w", encoding="utf-8") as f:
    f.write("\n".join(out) + "\n")

h = []
h.append("/* FICHIER GÉNÉRÉ PAR tools/gen_data.py — NE PAS ÉDITER À LA MAIN */")
h.append("#ifndef DATA_IDS_H\n#define DATA_IDS_H")
h.append("#define MOVE_TOTAL %d" % len(MOVES))
h.append("#define BIOME_COUNT %d" % len(BIOMES))
for i, m in enumerate(MOVES):
    h.append("#define M_%s %d" % (m[0], i))
for i, e in enumerate(EFFECTS):
    h.append("#define EF_%s %d" % (e, i))
h.append("#endif")
with open(os.path.join(ROOT, "include", "data_ids.h"), "w", encoding="utf-8") as f:
    f.write("\n".join(h) + "\n")

# ------------------------------------------------ vérifications
for d in range(1, 152):
    n, t1, t2, st = SPECIES[d - 1]
    assert all(1 <= x <= 255 for x in st), n
    if d in EVO:
        nx, lv = EVO[d]
        assert 1 <= nx <= 151 and lv > 0 and nx != d
total = sum(1 for d in range(1, 152) if d not in prev)
print("OK : %d espèces, %d attaques, %d familles de départ, %d biomes" % (len(SPECIES), len(MOVES), total, len(BIOMES)))
for d in (1, 6, 25, 94, 130, 143, 149, 150):
    n, t1, t2, st = SPECIES[d - 1]
    print("  #%03d %-10s %-9s/%-9s total=%d  coût=%d" % (d, n, TYPES[t1], TYPES[t2] if t2 != 255 else "-", sum(st), costs[d]))
