Program ipk-sniffer.cpp zachytává pakety a vypisuje hlavičku a data, které jsou v paketu obsažené.
Zdrojový soubor se přeloží pomocí příkazu "make", je-li k dispozici soubor Makefile. Spuštění probíhá
ve formátu: sudo ./ipk-sniffer argumenty. Je nutné dát programu dostatečná práva pomocí příkazu sudo.
Argumenty mohou být zadány v jakémkoliv pořadí. 
Argumenty:
	-i <string>  : Rozhraní na kterém se mají pakety hledat. V případě nepřítomnosti argumenty se vypíšou
		      všechna dostupná rozhraní.
		
	-p <int>     : Port na kterém se mají pakety hledat. V případě nepřítomnosti hledá na všech portech. 
	
	-n <int>     : Počet hledaných paketů. V případě nepřítomnosti se hledá právě jeden paket.

	--tcp nebo -t : Hledají se pouze tcp pakety.

	--udp nebo -u : Hledají se pouze udp pakety.

	-help	      : Vypíše pomocný text pro uživatele, který jej směřuje k přečtení README pro další informace.

V případě jiného argumentu se program ukončí s chybovou zprávou. Pokud bude zadán jak argument --tcp a --udp,
bude program fungovat jako by nebyl zadán ani jeden z těchto argumentů.

Spouštět přeložený program můžete například takto:
sudo ./ipk-sniffer -i enp0s3 -p 80 -n 5 -t

Program nepřekládá ip adresy na FQDN z důvodu častého zacyklení programu. V některých případech program zachytával
pakety které měli za úkol adresu přeložit. Poté se program pokusil přeložit ip adresu těchto pakétů a program se 
zacyklil.

Odevzdáné soubory:
	ipk-sniffer.cpp
	Makefile
	manual.pdf
	README
