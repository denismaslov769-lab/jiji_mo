-- 005: больница All Saints получает собственный интерьер (int 21, мир 7),
-- точки сгенерированы tools/interiors/gen_interiors.py
INSERT OR REPLACE INTO locations VALUES ('med_reception',2605.47,-2895.80,1501.00,0,21,7,'Больница');
INSERT OR REPLACE INTO locations VALUES ('hospital_in',2614.45,-2898.80,1501.00,0,21,7,'Выход из больницы');
INSERT OR REPLACE INTO locations VALUES ('hospital_lobby',2611.00,-2895.00,1501.00,90,21,7,'Холл больницы');
INSERT OR REPLACE INTO locations VALUES ('med_ther',2603.90,-2886.60,1501.00,0,21,7,'Больница');
INSERT OR REPLACE INTO locations VALUES ('med_eye',2609.60,-2887.00,1501.00,0,21,7,'Больница');
INSERT OR REPLACE INTO locations VALUES ('med_lab',2616.05,-2886.40,1501.00,0,21,7,'Больница');
INSERT OR REPLACE INTO locations VALUES ('hospital_spawn',2622.10,-2884.40,1501.00,0,21,7,'Палата больницы');
ALTER TABLE houses ADD COLUMN interior INTEGER DEFAULT -1;
