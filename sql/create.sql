runtime zum integrieren verwenden?

# F�r Spalten mit NULL wird zus�tzlich Speicher ben�tigt! wir verzichten
# daher darauf.
# MyISAM Tabellen d�rften f�r unseren Zweck effizienter sein.

# Diese Werte rechnen wir der Konfiguration zu und loggen
# nur, wenn sich etwas ver�ndert hat:
CREATE TABLE vitolog_config (
      timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,  # 4 byte
      mode TINYINT(1) UNSIGNED NOT NULL,              # 0-2
      warmwasser_soll TINYINT(2) UNSIGNED NOT NULL,   # 1�
      warmwasser_offset TINYINT(2) UNSIGNED NOT NULL, # 1�
      raum_soll TINYINT(2) UNSIGNED NOT NULL,         # 1�
      raum_soll_red TINYINT(2) UNSIGNED NOT NULL,     # 1�
      neigung TINYINT UNSIGNED NOT NULL,              # 0,1
      niveau TINYINT UNSIGNED NOT NULL,               # 1K
   ) ENGINE = MyISAM, ROW_FORMAT=FIXED;
   
# Hier wird nur geloggt, wenn der Brenner oder Pumpe auch l�uft, aber daf�r
# mit hoher Aufl�sung (10sec.):
CREATE TABLE IF NOT EXISTS vitolog_kessel (
      timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL PRIMARY KEY,  # 4byte
      start_count INT UNSIGNED NOT NULL,           # Z�hler (4byte)
      ventil TINYINT(1) UNSIGNED NOT NULL,         # 0-3
      pumpe TINYINT(3) UNSIGNED NOT NULL,          # %
      flow SMALLINT UNSIGNED NOT NULL,             # l/h
      leistung TINYINT(3) UNSIGNED NOT NULL,       # %
      kessel_soll SMALLINT(3) NOT NULL,            # 0,1 �C
      kessel_ist SMALLINT(3) NOT NULL,             # 0,1 �C
      abgas SMALLINT(3) NOT NULL,                  # 0,1 �C
      INDEX( start_count )
   ) ENGINE = MyISAM, ROW_FORMAT=FIXED;
	    

# Hier kommen Werte die sich langsam ver�ndern und werden daher nur seltener
# geloggt, daf�r aber unabh�ngig vom Betriebszustand (60sec.):
CREATE TABLE vitolog_slow (
      timestamp TIMESTAMP CURRENT_TIMESTAMP NOT NULL PRIMARY KEY,   # 4 byte
      runtime INT UNSIGNED NOT NULL,                  # Z�hler, Sekunden, 4byte
      warmwasser_ist SMALLINT(3) UNSIGNED NOT NULL,   # 0,1�C
      aussentemp SMALLINT(3) NOT NULL,                # 0,1�C
      aussentemp_ged SMALLINT(3) NOT NULL,            # 0,1�C
      vorlauf_soll SMALLINT(3) NOT NULL               # 0,1�C
      kessel_ist SMALLINT(3) NOT NULL,                # 0,1�C (redundant in vitolog_kessel)
   ) ENGINE = MyISAM, ROW_FORMAT=FIXED;


# Hier wird der Fehlerspeicher geloggt, aber auch nur wenn sich etwas
# ge�ndert hat:
CREATE TABLE vitolog_errors (
      timestamp TIMESTAMP CURRENT_TIMESTAMP NOT NULL,   # 4 byte
      error1 TINYINT UNSIGNED NOT NULL,
      error2 TINYINT UNSIGNED NOT NULL,
      error3 TINYINT UNSIGNED NOT NULL,
      error4 TINYINT UNSIGNED NOT NULL,
      error5 TINYINT UNSIGNED NOT NULL,
      error6 TINYINT UNSIGNED NOT NULL,
      error7 TINYINT UNSIGNED NOT NULL,
      error8 TINYINT UNSIGNED NOT NULL,
      error9 TINYINT UNSIGNED NOT NULL,
      error10 TINYINT UNSIGNED NOT NULL,
      errortext VARCHAR(600) NOT NULL
   );

