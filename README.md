# Organigrammes du banc d'essai

Organigrammes du programme de la carte PCB du banc d'essai (Arduino Mega) et de son dialogue avec l'interface de supervision, dessinés avec les symboles normalisés ISO 5807 (NF Z 67-010).

**Site en ligne :** https://abdelilahmimid.github.io/organigrammes-banc/

## Équipe

- [@abdelilahmimid](https://github.com/abdelilahmimid), responsable du dépôt
- [@ighmorimane](https://github.com/ighmorimane)
- [@mohamedhenallu](https://github.com/mohamedhenallu)

## Contenu du dépôt

| Fichier | Rôle |
|---|---|
| `index.html` | Toute la page : textes, organigrammes et éditeur intégré, dans un seul fichier. |
| `README.md` | Ce mode d'emploi. |
| `.github/CODEOWNERS` | Désigne @abdelilahmimid comme relecteur obligatoire de chaque modification. |

La page contient quatre parties :

1. **Vue simplifiée** : réception des données (Arduino Mega vers interface PC) et transmission de la consigne (interface PC vers Arduino Mega puis carte PCB).
2. **Programme de l'Arduino Mega** : éléments connectés par la carte PCB (V1) et programme principal `setup()` / `loop()`.
3. **Sous-programmes** : interruption INT0, calcul de la vitesse, lecture et traitement des commandes du PC, sonde DS18B20, sécurité thermique, ventilation, frein à hystérésis, trame DATA.
4. **Réaction carte Mega / interface** : partie A (réception des données) et partie B (consigne et commande du frein à hystérésis).

## Règle de travail

La branche `main` est protégée : personne ne peut y écrire directement. Chaque modification passe par une **pull request**, que @abdelilahmimid relit et accepte.

Pourquoi : le site est publié automatiquement à partir de `main`. Tout ce qui arrive sur `main` est en ligne une minute plus tard, erreurs comprises.

## Modifier un organigramme avec l'éditeur de la page

1. Ouvre le site en ajoutant `#auteur` à la fin de l'adresse :
   https://abdelilahmimid.github.io/organigrammes-banc/#auteur
2. Fais tes modifications : « Modifier » sous une figure, « Modifier les textes » pour les textes du projet. Elles restent dans ton navigateur tant qu'elles ne sont pas publiées.
3. Clique sur **« Télécharger la page à jour »** : tu obtiens un fichier `index.html` qui contient toute la page avec tes modifications.
4. Sur GitHub, dans ce dépôt : **Add file > Upload files**, dépose ton `index.html` (il remplace l'ancien).
5. En bas, choisis **« Create a new branch for this commit and start a pull request »**, donne un nom clair à la branche (par exemple `organigramme-ventilation`), puis **Propose changes**.
6. Dans la pull request, explique en une ou deux phrases ce que tu as changé et pourquoi, puis **Create pull request**.

## Modifier avec Git (VS Code ou ligne de commande)

```bash
git clone https://github.com/abdelilahmimid/organigrammes-banc.git
cd organigrammes-banc
git switch -c ma-modification        # une branche par sujet
# ... modifier index.html ...
git add index.html
git commit -m "Organigramme ventilation : seuil de démarrage corrigé"
git push -u origin ma-modification
```

Ouvre ensuite la pull request sur GitHub (bouton **« Compare & pull request »**).
