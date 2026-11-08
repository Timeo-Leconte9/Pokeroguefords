# PokéRogue DS — prototype v0.4

Projet homebrew Nintendo DS indépendant, compilable automatiquement par GitHub Actions.
Il s'agit d'un prototype jouable inspiré du genre roguelite de capture et de combats au tour par tour,
pas d'un port officiel de PokéRogue. Aucun code, sprite, son ou autre asset propriétaire de Pokémon/PokéRogue n'est fourni.

## Compilation automatique
1. Crée un dépôt GitHub.
2. Envoie **tout le contenu** de cette archive, y compris le dossier caché `.github`.
3. Ouvre l'onglet Actions et lance « Build PokéRogue DS ».
4. Télécharge l'artefact `PokeRogueDS-nds` à la fin du workflow.
5. Décompresse l'artefact : le fichier attendu est `PokeRogueDS.nds`.

## Commandes du prototype
- Haut/Bas : choisir une action.
- A : valider.
- B : revenir au menu précédent.
- Combat : Attaque, capacité, capture (si disponible), objet de soin, fuite.
- Les étages multiples de 5 déclenchent un boss.
- Les étages multiples de 20 déclenchent un boss majeur.
- Les récompenses apparaissent après les combats.

## Limites honnêtes
Cette version contient une base de jeu et une liste de 151 noms d'espèces pour les rencontres.
Elle ne reproduit pas les données officielles complètes : statistiques, talents, attaques, sprites,
évolutions et équilibrage restent simplifiés ou à développer. La progression est une version
prototype, et doit être testée sur émulateur/console après compilation.
