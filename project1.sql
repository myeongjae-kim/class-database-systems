use Pokemon;

/* 01 */
SELECT name
FROM Trainer
WHERE hometown = 'Blue City';

/* 02 */
SELECT name
FROM Trainer
WHERE hometown = 'Brown City' OR hometown = 'Rainbow City';

/* 03 */
SELECT name, hometown
FROM Trainer
WHERE name REGEXP '^[aeiou]+';

/* 04 */
SELECT name
FROM Pokemon
WHERE type = 'Water';

/* 05 */
SELECT DISTINCT type
FROM Pokemon;

/* 06 */
SELECT name
FROM Pokemon
ORDER BY name ASC;

/* 07 */
SELECT name
FROM Pokemon
WHERE name REGEXP '[s]$';

/* 08 */
SELECT name
FROM Pokemon
WHERE name REGEXP '[e].*[s]$';

/* 09 */
SELECT name
FROM Pokemon
WHERE name REGEXP '^[aeiou]+';

/* 10 */
SELECT type, COUNT(*)
FROM Pokemon
GROUP BY type;

/* 11 */
SELECT nickname
FROM CatchedPokemon
ORDER BY level LIMIT 3;

/* 12 */
SELECT AVG(level)
FROM CatchedPokemon;

/* 13 */
SELECT
  (SELECT max(level) FROM CatchedPokemon)
  - (SELECT min(level) FROM CatchedPokemon)
AS `The Max-Min level of catched pokemons`;

/* 14 */
SELECT COUNT(*) AS `# of pokemon names starting with [b-e]`
FROM Pokemon
WHERE name REGEXP '^[b-e]';

/* 15 */
SELECT COUNT(*) AS `# of pokemons whose type are not...`
FROM Pokemon
WHERE type NOT REGEXP '^(Fire|Grass|Water|Electric)$';

/* 16 */
SELECT t.name AS 'Trainer', p.name AS 'Pokemon', c.nickname AS 'Nickname of Pokemon'
FROM Trainer AS t
JOIN (
  SELECT owner_id, pid, nickname
  FROM CatchedPokemon
  WHERE nickname REGEXP ' '
) AS c ON c.owner_id = t.id
JOIN Pokemon AS p ON c.pid = p.id;

/* 17 */
SELECT name
FROM Trainer
WHERE id IN (
  SELECT owner_id
  FROM CatchedPokemon
  WHERE pid IN (
    SELECT id
    FROM Pokemon
    WHERE type='Psychic'
  )
);

/* 18 */
SELECT DISTINCT name, hometown
FROM Trainer JOIN (
  SELECT owner_id, AVG(level) AS avglevel
  FROM CatchedPokemon
  GROUP BY owner_id
  ORDER BY AVG(level) DESC
  LIMIT 3
) AS topthree ON Trainer.id = topthree.owner_id
ORDER BY topthree.avglevel DESC;

/* 19 */
SELECT t.name AS 'Trainer', c.n AS '# of pokemons'
FROM Trainer AS t, (
  SELECT owner_id, COUNT(*) AS n
  FROM CatchedPokemon
  GROUP BY owner_id
) AS c
WHERE c.owner_id = t.id
ORDER BY c.n DESC, t.name DESC;

/* 20 */
SELECT name, sangnok_pokemon.level
FROM Pokemon, (
  SELECT pid, level
  FROM CatchedPokemon
  WHERE owner_id IN (
    SELECT leader_id
    FROM Gym
    WHERE city='Sangnok City'
  )
) AS sangnok_pokemon
WHERE sangnok_pokemon.pid = id
ORDER BY sangnok_pokemon.level ASC;

/* 21 */
SELECT name AS Pokemon, COALESCE(catched, 0) AS 'Catched Count'
FROM Pokemon LEFT JOIN (
  SELECT pid, COUNT(*) AS catched
  FROM CatchedPokemon
  GROUP BY pid) AS catched_count
ON Pokemon.id = catched_count.pid
ORDER BY catched DESC;

/* 22 */
SELECT name
FROM Pokemon
WHERE id IN(
  SELECT after_id
  FROM Evolution
  WHERE before_id IN(
    SELECT after_id
    FROM Evolution
    WHERE before_id IN(
      SELECT id
      FROM Pokemon
      WHERE name='Charmander'
    )
  )
);

/* 23 */
SELECT name
FROM Pokemon
WHERE id IN (
  SELECT DISTINCT pid
  FROM CatchedPokemon
  WHERE pid <= 30
)
ORDER BY name ASC;

/* 24 */
/* 잡힌 포켓몬을 트레이너별로 분류 */
/* 분류된 포켓몬들의 타입이 2가지 이상인 트레이너 제외 */
/* 남아있는 트레이너와 타입을 출력 */

SELECT Trainer.name, Pokemon.type
FROM CatchedPokemon
JOIN Trainer ON CatchedPokemon.owner_id = Trainer.id
JOIN Pokemon ON CatchedPokemon.pid = Pokemon.id
GROUP BY Trainer.id
HAVING COUNT(DISTINCT Pokemon.type) = 1;


/* 25 */
SELECT Trainer.name, Pokemon.type, COUNT(*)
FROM Trainer
JOIN CatchedPokemon ON Trainer.id = CatchedPokemon.owner_id
JOIN Pokemon ON CatchedPokemon.pid = Pokemon.id
GROUP BY Trainer.name, Pokemon.type;

/* 26 */
SELECT one_pokemon_trainer.name AS Trainer, Pokemon.name AS Pokemon, catched_count AS 'Catched Count'
FROM (
  SELECT Trainer.name,pid,catched_count
  FROM (
    SELECT owner_id, pid, COUNT(CatchedPokemon.pid) AS catched_count
    FROM CatchedPokemon
    GROUP BY CatchedPokemon.owner_id
    HAVING COUNT(DISTINCT CatchedPokemon.pid) = 1) AS one_pokemon
  JOIN Trainer ON Trainer.id = one_pokemon.owner_id) AS one_pokemon_trainer
JOIN Pokemon ON pid = Pokemon.id;

/* 27 */
SELECT Trainer.name, Gym.city
FROM Trainer, Gym
WHERE Trainer.id = Gym.leader_id
AND Gym.leader_id IN (
  SELECT owner_id
  FROM (
    SELECT *
    FROM CatchedPokemon
    WHERE owner_id IN (
      SELECT leader_id
      FROM Gym
    )
  ) AS leaderPokemon JOIN Pokemon ON leaderPokemon.pid = Pokemon.id
  GROUP BY owner_id
  HAVING COUNT(type) != 2
);

/* 28 */
SELECT Trainer.name, SumOfLevel
FROM Trainer LEFT JOIN (
  SELECT owner_id, SUM(level) AS SumOfLevel
  FROM CatchedPokemon
  WHERE owner_id IN (
    SELECT leader_id
    FROM Gym
  ) AND level >= 50
  GROUP BY owner_id
) AS MoreThanFifty ON Trainer.id = MoreThanFifty.owner_id
WHERE id IN (
  SELECT leader_id
  FROM Gym
);

/* 29 */
SELECT name
FROM Pokemon
WHERE id IN(
  SELECT pid
  FROM CatchedPokemon JOIN (
    SELECT id AS owner_id, hometown
    FROM Trainer
    WHERE hometown = 'Sangnok City' OR hometown = 'Blue City'
  ) AS SangnokOrBlue ON CatchedPokemon.owner_id = SangnokOrBlue.owner_id
  GROUP BY pid
  HAVING COUNT(DISTINCT hometown) = 2
);

/* 30 */
-- below query can handle fourth evolution.
SELECT name
FROM Pokemon
WHERE id IN (
  SELECT DISTINCT COALESCE(Evolution.after_id, thirdTable.thirdID) AS fourthID
  FROM Evolution RIGHT JOIN (
    SELECT DISTINCT COALESCE(Evolution.after_id, secondTable.secondID) AS thirdID
    FROM Evolution RIGHT JOIN (
      SELECT after_id AS secondID
      FROM Evolution
    ) AS secondTable ON Evolution.before_id = secondTable.secondID
  ) AS thirdTable ON Evolution.before_id = thirdTable.thirdID
)
;
