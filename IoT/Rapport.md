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
(Note: Cette partie du rapport sera moins détaillé, il y a beaucoup de travail à faire en cette fin de semestre)

### Préparation des interruptions
Dans cette première partie nousa allons voir comment activer les interruptions et les utiliser:

Dans un premier temps nous devons modifier le fichier ***kernerl.ld*** afin d'allouer un espace en mémoire suffisant pour une seconde pile, dédiée aux interruptions. L'adresse de base de cette pile est stocké dans la variable __irq\_stack\_top__ .
Ensuite nous y ajoutons aussi le code du fichier irq.s (dans la partie .text) qui contient des définitions pour les fonctions de mise en attente d'une interruption pour le processeur (__\_wfi__), de la mise en palce des interruptions au niveau du processeur (__\_irqs\_setup__) et d'activation et de désactivation des interruptions, toujours au niveau du processeur.

La prochaine étape consiste à activer les interruptions au niveau du VIC (le contrôleur d'interruptions). Pour cela il faut éditer les fichiers ***isr.c***, ***isr.h*** et ***isr-mmio.h***. Dans les ifhcier ".h", nous définissons différentes constantes notamment les adressess de plusieurs registres du VIC, permettants de gérer quelles interruptions sont actives (en IRQ, FIQ, ou pour voir quelles interruptions ont été activée et effectivement reçues (encoding "one hot")), ainsi que certains regsitres permettant de désactiver les interruptions. Il y en aura donc une par bit soit 32 interruptions possibles (on retrouve des timers, les uarts, etc..., les périphérique connectés sont trouvables dans la documentation de la board, puisque le constructeur choisit les branchements). Les adresses de ces registres sont trouvables dans la documentation du VIC.
On définit aussi un ensemble de masques correspondant aux adresses auxquelles sont branchés les composants de la board ainsi que le numéro du bit correspondant à chaque interruptions qui nous intéressent (par exemple sur la board du cours on trouve que l'interruption de l'UART0 est associé au bit 12 des registres du VIC).

Enfin, nous allons devoir faire quelques changements dans le fichier ***uart.c*** de manière à activer les interruptions au niveau des UARTs. Lors de l'initialisation de chaque UART nous remettons à zéro le registre __IMSC__. Ce registre indique et modifie quelles interruptions de l'UART sont activées. Nous devrons donc modifier ce registre au cours de l'exécution du programme pour stopper les interruptions pendant qu'un traitant est en cours.

### Handlers
Nous pouvons maintenant définir des handlers pour nos différentes interruptions. La première chose à faire est d'indiquer la fonction à appeler lorsqu'une interruptions arrive. C'est au sein de cette fonction que nous pourrons savoir quelles interruptions (grâce au registre IRQSTATUS, en lisant ses bits, on peut connaître l'origine de l'interruptions) sont arrivées et la fonction à exécuter dans chaque cas. Cette fonction est __isr()__, dans le fichier ***isr.c*** et dans le fichier ***exception.s*** nous définissons la fonction __\_isr\_handler__ qui sauvegarde dans la pile tout les registres et le lr, puis se branche sur la fonction __isr()__. Au retourn de cette fonction tout les registres sont rechargés.

On défini maintenant les actions spécifique à chaque interruptions. Dans notre cas nous nous concentrons sur les interruptions de UART. La fonction que nous utiliserons lira le caractères reçu depuis l'UART et le renverra, de la même manière que dans la première étape. Dans la boucle infinie du __start()__ nous remplacopns le code par un appel à __core\_halt()__. Désormais la réception et l'envoi de caractère ne se fera que lorsqu'une interruption sera envoyée par l'UART et le processeur restera dans un mode basse consommation en attendant.

Au moment où j'écris ces lignes l'analyse de l'encodage des caractères et de certaines chaînes n'a pas pû être fais par manqua de temps. La seule chose qui a été remarqué est que les flèches semble redémarrer le programme.

## Troisième semaine

(Disclaimer) Il semblerait que le comportement des flèches soit en réalité un bug.

### Programmation evénementielle

Dans cette troisième partie nous allons mettre en place une interface pour faciliter le développement d'un programme sur la carte. Il s'agit désormais de cacher le comportement des UARTs lors des lectures et écritures, en masquant les activation et désactivation des interruptions. Nous voulons aussi limiter le risque de perte de données en bufferisant les écritures et lectures de l'UART. De fait, le nouveau système mettra en place des évenements auxquels "s'abonneront" des listeners. Ce sera à l'utilisateur de définir le comportement de ces listeners en profitant de l'interface fournie.

La fonction __uart\_init__ à pour but d'associer à chaque uart un listener à appeler lorsque des données sont disponibles dans le buffer de réception, c'est à dire lorsque le buffer est non-vide (__read\_listener__ == __rl__), et un listener appelé lorsqu'il est possible d'écrire dans le buffer d'émission (__write\_listener__ == __wl__), c'est à dire lorsque le taux de remplissage du buffer à passé un seuil. Les fonctions __uart\_read__ et __uart\_write__ seront utilisées au mieux dans le contexte de ces listeners (définis par l'utilisateur). Par conséquent chaque UART disposera de deux buffers (circulaire pour éviter des problèmes de synchronisation) un pour l'émission et l'autre en réception.

Voici le fil d'exécution des évènements:

1. Une interruption arrive 
2. Le traitant d'interruptions va (selon l'interruptions) lancer une lecture en stockant le résultat dans le buffer de lecture ou tenter une écriture depuis le buffer d'écriture, associé à l'UART (fonction de la semaine 2)
3. Quand l'opération aboutit, selon l'état des buffers le listener associé est appelé (en attendant d'avoir un liste d'event).

La solution est donc:

1. Réutiliser les interruptions comme réalisées plus haut. Modifier la structure d'UART pour prendre en compte les deux buffers.
2. Modifier le traitant pour lire/ écrire et mettre à jour les buffers correctement.
3. Appeler dans le traitant les listeners lorsque nécessaire

