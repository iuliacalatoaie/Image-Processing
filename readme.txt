Calatoaie Iulia-Adriana
Grupa 335CA

Tema 3 APD

-implementarea pe mai multe procese a filtrarii unei imagini implica comunicarea
dintre procesul master, in acest caz, ultimul, cu restul proceselor, dar si
proceselor intre ele
-master citeste imaginea, imparte pe linii matricea de pixeli, trimitand liniile
impreuna cu celelalte date necesare(ex. numarul de linii, coloane, filtre,
liniile alocate impreuna cu doua linii de bordare pentru a putea calcula
produsul de convolutie) celorlalte procese; lui ii vor ramane ultimele linii
nealocate pentru prelucrare
din matrice
-fiecare proces va primi datele, isi va calcula liniile de prelucrare, va aplica
produsul de convolutie pe fiecare pixel si, daca sunt filtre multiple aplicate,
va face schimb cu procesele vecine de rang -/+ 1 fata de rangul curent pentru a
face update la liniile de bordare, respectiv liniile deja prelucrate de
procesele vecine
-pentru a evita deadlock, conventia aleasa este ca rangurile pare sa trimita
liniile si mai apoi sa le primeasca, iar rangurile impare invers


