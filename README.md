# Précisions :
-Ce n' est pas les mêmes textures que l' original

-Il n' y a pas encore de système pour sauvegarder 

-Les sprites sont statiques et sont fait à partir des catégories de formes des pokémons

-Il y a toute la 1ere génération (même les évolutions peuvent être prises en starter)

-L' équilibrage est totalement inexistant, donc j' ai testé Mewtwo avec le max de reste (il y a quand même une limite), c' est plutôt fort

-Le jeu tourne très bien, ça c' est sûr 

-Qu' un bug trouvé, lors de la sélection du prochain biome l' écran devient noir

-Il y a les mégas, le dynamax et la téra, mais on peut l' utiliser a chaque combat

-Aucun système de gacha pour l' instant, ni d' achievement, ni de challenge, ni de fusion, juste du classique jusqu'au palier 200

-Ca m' étonne que t' as lu jusqu' ici

-Le début est très facile

-Ca n' est pas en pleine écran

Bref, si vous ne voulez pas que l' ia se développe et commence a faire des jeux pareils, n' y jouez pas. Merci !


# PokéRogue DS (inspiration nes je pense)

Roguelike inspiré de PokéRogue pour Nintendo DS / 2DS / 3DS (mode DS), en C avec devkitARM + libnds.

## Compiler
1. Installer devkitPro (devkitARM + `nds-dev`, qui contient libnds et libfat).
2. `make` → produit `PokeRogueDS.nds`.
3. Copier le `.nds` sur la carte SD (flashcard / TWiLight Menu++ / hbmenu). Les sauvegardes (`PokeRogueDS.sav`) et réglages (`PokeRogueDS.cfg`) sont écrits à côté via libfat.

## Commandes
Tout se fait au stylet ou avec les touches : A valide, B annule, croix directionnelle déplace.
En combat, sur l'écran d'attaques : boutons tactiles **MÉGA**, **DYNAMAX**, **TÉRA** (ou touche **X** pour les faire défiler), puis l'attaque.
Menu titre : **Vitesse x1 à x4** (mémorisée).

## Options (menu titre > Options)
- Vitesse du jeu x1 à x4.
- Musique de combat : Auto, Classique, Route, Épique (3 morceaux originaux synthétisés) ou **Perso (SD)**.
- Musique activée / coupée.

### Musique perso (usage personnel)
Convertis ton propre fichier audio en PCM brut 8 bits signé, mono, 16 kHz, et pose-le à côté du `.nds` sous le nom `PokeRogueDS_music.raw` (1,2 Mo max, soit ≈ 75 s) :

    ffmpeg -i ma_musique.mp3 -ac 1 -ar 16000 -t 75 -f s8 PokeRogueDS_music.raw

Aucune musique officielle n'est fournie avec le projet.

## Contenu
- 151 Pokémon (stats modernes à 6 stats, types, évolutions), 136 attaques, 14 biomes, boss toutes les 10 vagues, 200 vagues.
- Talents simplifiés déduits du type (Brasier, Torrent, Engrais, Intimidation, Lévitation, Statik, Corps Ardent, Point Poison, Fermeté, Cran).
- Capture (Poké, Super, Hyper, Rogue et Master Ball), objets tenus, boutique (3 objets aléatoires selon la rareté à gauche, Potion / Rappel / Élixir toujours en vente à droite), récompenses, équipe de 6, sauvegarde.
- Méga-évolutions (Florizarre, Dracaufeu X/Y, Tortank, Dardargnan, Roucarnage, Alakazam, Flagadoss, Ectoplasma, Kangourex, Scarabrute, Léviator, Ptéra, Mewtwo X/Y), Dynamax (PV x2, 3 tours, attaques Dyna), Téracristallisation (type de l'attaque choisie, STAB x2/x1,5).
- Graphismes et sprites **générés par le code** (aucune ressource Nintendo), musiques chiptune synthétisées.

## Limites (honnêtes)
- Ce n'est pas un clone « parfait » : la DS n'a que 4 Mo de RAM et aucune ressource officielle n'est incluse.
- Le code a été testé sur PC avec un faux libnds + robot de test, **pas** sur un vrai devkitARM ni du vrai matériel. Les 60 fps (≥ 35 garantis même en x4) viennent d'un rendu par zones modifiées, non mesuré sur console.
- Les listes d'attaques apprises sont générées par type, pas les vraies.
- Les musiques n'ont pas été écoutées sur DS.
- Dynamax : pas d'effets secondaires des attaques Dyna ; ennemis sans Méga/Dynamax/Téra.
