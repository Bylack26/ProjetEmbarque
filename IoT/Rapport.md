# Développement embarqué

## Première semaine

On commence cette semaine avec un bout simple de code. On dispose du code source et d'un fichier Makefile

### Makefile

Le Makefile défini sur les première lignes (17-23) des constantes pour compiler le code pour une board ("versatile") avec un certain CPU ("cortex-a8").

Ensuite on retrouve d'autres constantes qui vont servir à lancer l'emulateur et à configurer la board à émuler (lignes 31-47), en désactivant l'affichage, en choissant une taille pour lé mémoire.

En précisant l'option __-serial mon:stdio__ cela nous permet de passer en __Record mode__ et de n'enregistrer que les entrées de notre ordinateur non déterministe c'est à dire qui ne peuvent être prévues (Clavier, souris). L'autre mode possible, __Replay__, avec l'option __-serial null__ enregistre toutes les entrées, même déterministes, comme par exemple les lectures en mémoire, ou sur disque. La différence est que ces entrées déterminsite peuvent être rejouées.

On y déclare aussi les flags pour la compilation en précisant le cpu visé et en précisant l'option __-ffreestanding__ on n'utilisera pas de couche de système d'exploitation par dessus notre Hardware. Nous n'utiliserons pas non plus la librairie standard avec l'option __-nostdlib__

Enfin les options suivante seront utilisées lors de l'étape de linkage. Encore une fois nous désactivons la librairie standard et cette fois-ci nous allons utilisé un fichier personnel pour le linkage avec l'option __-T__. Nous précisons aussi que nous ne souhaitons pas utilisé le linkage dynamique avec l'option __-static__. En mode statique, l'éxécutable contiendra toute les librairies nécessaires et il ne sera donc pas nécessaire de charger ces librairies. Cela sera préférable dans le cas du développement embarqué pour savoir quelle taille le programme prendra en mémoire. 

On voit aussi dans le code du linker que les codes assembleurs de ***exception.s*** et ***startup.s*** seront écrit dès le début de la zone texte de notre fichier (c'est pour cela que les instructions de ces deux fichier sont le première à s'exécuter quand l'on regarde dans gdb).

On définit ensuite les règles de compilation. Outre les règles classiques pour construire les fichier objets, on retrouve aussi deux règles pour compiler et lancer le programme. La première, __run__, permet de lancer le programme normalement sur l'émuateur __qemu__. La règle __debug__, elle, ajoute les options __-gdb tcp:1234__, signifiant l'ouverture du port 1234 selon le protocole TCP pour y connecter gdb et ainsi pouvoir débugger notre programme, et l'option __-S__, pour stopper l'exécution du programme avant la première instruction (ce qui est nécessaire pour débugger correctement).

### Le code source

Le fichier ***main.c*** va contenir une fonction _start qui sera la première lancée. En s'en assure dans le fichier ***startup.s***, où les dernières lignes indiquent:
```
ldr r3,=_start
blx r3
```
signifiant que l'on stocke dans le registre 3 l'adresse de la fonction __\_start__ (***=_start*** signifie l'adresse de _start) puis que nous effectuons un saut à cette adresse avec l'instruction __blx__. Cette instruction permet aussi de stocker l'adresse de retour (adresse de l'instruction suivante) pour revenir lorsque la fonction appelée termine.

Le début du fichier ***startup.s*** définit une liste de constantes, correspondants à des masques pour le registre __CPSR__. Les 32 bits de ce registre définissent diverses informations comme par exemple le mode dans lequel le processeur va fonctionner (dans ce cas en mode système, avec un accés total à la mémoire et au registre), en modifiant les bits de 0 à 4 pour définir le mode d'exécution.  
On modifie aussi les bits allant de 5 à 7 permettant de définir: en 5 le mode ARM ou Thumb, en 6 l'activation des interruptions de type Fast Interrupt (FIQ) et en 7 le flag pour les interruptions IRQ. Nous désactivons ces deux types d'interruptions en mettant les bits 6 et 7 à 1. Pour ce qui est du mode ARM et Thumb, pour l'instant nous savons qu'il s'agit d'une différence sur le nombre de bits pour coder une instruction (32 en ARM et 16 en Thumb). De fait le nombre d'instructions est plus limité.

La fonction __\_start__ est une boucle infini appelant deux fonction successivement: __uart\_receive__ et __uart\_send__, définis toutes deux dans le fichier ***uart.c***. Ces deux fonctions vont lire et écrire avec une interface UART de notre board. Cette interface permet de communiquer entre un périphérique (ici notre PC) et notre board avec un "câble" série (Il n'y aura pas de cable dans notre cas, car nous emulons la board) permettant d'envoyer et de recevoir des données par le même fil. 

Comme vu dans le cours chaque interface UART correspond en réalité à une zone mémoire de 4KiB. Chacunes de ces zones mémoire est découpée en petite région. On définit donc dans le fichier ***uart-mmio.h***  l'adresse de base de ces interfaces, que l'on peut retrouver dans la documentation de la board (cette adresse peut changer en fonction du matériel). Et pour les besoins de ce TP, nous définissons aussi les deux constantes __UART\_DR__ (0x00) et __UART\_FR__ (0x18), deux offset à partir de l'adresse de base de l'interface UART, permettant d'accéder respectivement, à la zone où les données peuvent être écrites et lues (Data Register), et à la zone contenant les flags relatifs à l'interface UART (Flag Register). Ces informations peuvent être retrouvées dans la documentation de UART_PL011

Nous lisons les flags du __UART\_FR__ (il s'agit d'un registre de 32 bits). Chaque bit donne une information spécifique. Dans le cadre de ce premier exercice les bits qui nous intéressent sont les bits 3,4 et 5.  
Le bit 3 indique si la file d'émission est encore occupé à envoyé un paquet. On utilise ce bit pour rendre l'envoi de données bloquant.  
Le bit 4 indique si la file de réception est vide. Auquel cas il n'y a pas de données à lire en réception depuis l'ordinateur.  
Enfin le bit 5 indique si la file d'émission est pleine. On peut ainsi éviter d'envoyer des données qui seraient perdues autrement.

### Lancer le programme

Pour lancer le programme nous commençons par utiliser la commande: ***make run*** qui va à la fois compiler le programme et lancer l'émulateur avec l'exécutable ainsi construit. On peut alors taper des caractères dans le terminal et ceux-ci seront envoyés à la "board". Ensuite les caractères seront renvoyés vers notre terminal.  
Si nous souhaitons débugger on utilisera la commande: ***make debug*** puis nous commanderons l'exécution du programme en utilisant gdb-multiarch, une version de gdb fonctionnant avec les exécutables compilé pour ARM:
***gdb-multiarch ./build/kernel.elf***
et en se connectant à la board par tcp avec la commande ***target remote :1234***, signifiant que nous nous connectons à un programme sur le port 1234 de localhost. Nous pouvons alors envoyer des commandes pour manipuler l'exécution de notre programme.

### Les fonctions __inline__

Dans le fichier main.h, il y a des fonctions notés __\_\_inline\_\___. Cela signifie que lors de la compilation, lorsque ces fonctions sont appelées, au lieu de mettre un appel à cette fonction le compilateur va plutôt "copier" le corps de la fonction à l'endroit où elle est appelée (__\_\_attribute\_\_((always\_inline))__ force le compilateur à ***inliner*** la fonction même si ce n'est pas optimisé). Ca évite de mulitplier les sauvegardes de contextes et dans le cas de la programmation embarquée, la taille de la pile n'est pas aussi importante que sur un PC.
 

## Deuxième semaine

### Interruptions au niveau de UART

Registres pour les interruptions de UART
UARTIMSC 0x38
UARTRIS 0x3C
UARTMIS 0x40
UARTICR 0x44

### Interruptions au niveau du VIC

Adrese du VIC 0xFFFFFF00

### Interruptions au niveau du CPU

