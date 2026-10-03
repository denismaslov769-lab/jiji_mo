-- 2.2.1: метка к мотелю Джефферсон на шаге квеста «Арендуй жильё»
INSERT OR REPLACE INTO locations VALUES ('motel',2230.0,-1166.5,25.73,0.0,0,0,'Мотель Джефферсон');
UPDATE quest_steps SET loc='motel', hint='Едь по флажку на карте к мотелю Джефферсон. Подойди к двери комнаты и нажми ALT ($60).' WHERE quest=1 AND step=15;
