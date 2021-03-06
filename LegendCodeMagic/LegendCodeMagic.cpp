#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <array>
/*
Notes about exploring every possible turn

The thing about this exploration is that there is lots of times where the order of actions won't matter
This can really help reduce the search space. For example, the order of playing minions doesn't matter.
Since there is no AOE damage, it's hard to see a reason why you would ever hold back on playing a minion if you have the mana / space
Actually, this is plausible. You might not want to play a 9/1 into a 1/1, you would rather just hold it a lot of times
It is also completely plausible that might want to save an item for a later turn even if it's playable, or that you may want to use it halfway through an attack phase
For example, you may want to use a 1/1 to break a ward, then use an item to deal damage to it, and then attack with the rest of your minions

I see arguments for both a MonteCarlo search and pruned search.
But how would Monte Carlo work in this context?
If I give it the option of holding onto minions it has mana for, and weaving items into the attacking phase, then the possibilities are huge
There's also the possibility for a large number of move sequences that are essentially identical

I think I'm leaning more towards full pruned search, and here's why:
	This is a highly tempo-based game. There won't be many times when holding back on spending mana is a good idea
	The times when it's optimal to weave an item into the middle of the attacking phase is rare. Worst case, I can avoid drafting these kinds of items
	If I heuristically pick decent moves first, then I'll likely have a good turn even if I don't have time to search every option

These are the discrete things I need to search:
	How I will spend my mana. Never hold back on mana. Find all the ways I can spend it, order doesn't matter, split into minions and items
	How I will do minion combat if my played cards won't affect it. This is true as long as I don't play charge minions or green/red items
	Acutally this isn't neceassarily true
	For each option:
		If there's charge minions, play them right away
		If I'm using blue items, use them
		If I'm using green items, try on every possible friendly minion
		If I'm using red items, try on every possible enemy minion
		Then do full combat branching
		Summon remainder of minions
		Full combat branching for opponent
		Evaluate




*/

using namespace std;

enum ABILITY { BREAKTHROUGH, CHARGE, GUARD, DRAIN, LETHAL, WARD };
enum ITEMTYPE { MINION, GREEN, RED, BLUE };

struct Card
{
	int number;
	int ID;
	int cost;
	int attack;
	int defense;
	int offer;
	int draw;

	bool breakthrough;
	bool charge;
	bool guard;
	bool drain;
	bool lethal;
	bool ward;

	bool canAttack;
	ITEMTYPE type;

	Card()
		: breakthrough(false)
		, charge(false)
		, guard(false)
		, drain(false)
		, lethal(false)
		, ward(false)
	{}
};


struct Player
{
	int health;
	int mana;
	int deckSize;
	int Rune;
	int handSize;
	vector<Card> handCards;
	vector<Card> boardCards;

	vector<vector<Card*>> allPossiblePlays(int startIndex, int inputMana, array<int, 8> & maxCosts)
	{
		// This is a recursive problem
		vector<vector<Card*>> plays;
		for (int i = startIndex; i < handSize; i++)
		{
			Card& card = handCards[i];
			if (card.cost <= inputMana)
			{
				vector<vector<Card*>> subPlays = allPossiblePlays(i + 1, inputMana - card.cost, maxCosts);
				for (auto& play : subPlays)
				{
					play.push_back(&card);

					// Only add the play if it's greater than the maxCost for this start index
					int mana = 0;
					for (auto card : play)
					{
						mana += card->cost;
					}

					if (startIndex != 0 || mana > maxCosts[i])
					{
						plays.push_back(play);
					}
				}

				if (inputMana > maxCosts[i])
				{
					maxCosts[i] = inputMana;
				}
			}
		}

		// Doing nothing is only an option when you no longer have mana to play any of your cards
		if (plays.empty())
		{
			plays.emplace_back();
		}

		

		return plays;
	}
};

int main()
{
	Player hero, enemy;
	// game loop
	while (1) {
		for (int i = 0; i < 2; i++) {
			int playerHealth;
			int playerMana;
			int playerDeck;
			int playerRune;
			cin >> playerHealth >> playerMana >> playerDeck >> playerRune; cin.ignore();
			Player& player = i == 0 ? hero : enemy;
			player.health = playerHealth;
			player.mana = playerMana;
			player.deckSize = playerDeck;
			player.Rune = playerRune;
		}
		int opponentHand;
		cin >> opponentHand; cin.ignore();
		enemy.handSize = opponentHand;
		int cardCount;
		hero.boardCards.clear();
		hero.handCards.clear();
		enemy.boardCards.clear();
		cin >> cardCount; cin.ignore();
		for (int i = 0; i < cardCount; i++) {
			int cardNumber;
			int instanceId;
			int location;
			int cardType;
			int cost;
			int attack;
			int defense;
			string abilities;
			int myHealthChange;
			int opponentHealthChange;
			int cardDraw;
			cin >> cardNumber >> instanceId >> location >> cardType >> cost >> attack >> defense >> abilities >> myHealthChange >> opponentHealthChange >> cardDraw; cin.ignore();

			Card card;
			card.number = cardNumber;
			card.ID = instanceId;
			card.type = ITEMTYPE(cardType);
			card.cost = cost;
			card.attack = attack;
			card.defense = defense;
			card.offer = i;

			for (auto& ab : abilities)
			{
				if (ab == 'B') { card.breakthrough = true; }
				if (ab == 'C') { card.charge = true; }
				if (ab == 'G') { card.guard = true; }
				if (ab == 'D') { card.drain = true; }
				if (ab == 'L') { card.lethal = true; }
				if (ab == 'W') { card.ward = true; }
			}

			card.draw = cardDraw;

			if (location == 0)
			{
				card.canAttack = card.charge ? true : false;
				hero.handCards.push_back(card);
			}
			else if (location == 1)
			{
				card.canAttack = true;
				hero.boardCards.push_back(card);
			}
			else
			{
				card.canAttack = true;
				enemy.boardCards.push_back(card);
			}

		}

		hero.handSize = hero.handCards.size();

		if (hero.mana == 0)
		{
			// Draft
			// Cheapest card for now, most attack, bonus for charge
			// Maybe most attack per mana

			int bestNum = -1;
			float bestAPM = -1.0f;
			for (auto& card : hero.handCards)
			{
				float weight = card.attack + 0.3f * card.defense;
				if (card.type == RED) { weight = -10.0f; }

				if (card.charge || card.breakthrough)
				{
					weight *= 1.3f;
				}
				if (card.guard)
				{
					weight += 1.0f;
				}
				if (card.ward)
				{
					weight += 2.0f;
				}
				if (card.lethal)
				{
					weight += 2.0f;
				}
				if (card.drain)
				{
					weight += 1.0f;
				}

				weight += card.draw * 2;

				if (weight / (float)(card.cost) > bestAPM)
				{
					bestNum = card.offer;
					bestAPM = weight / (float)(card.cost);
				}
			}

			cout << "PICK " << bestNum << endl;

		}

		else
		{
			//Test play finder
			array<int, 8> maxCosts = { {-1,-1,-1,-1,-1,-1,-1,-1} };
			vector<vector<Card*>> plays = hero.allPossiblePlays(0, hero.mana, maxCosts);
			for (auto& play : plays)
			{
				for (auto& card : play)
				{
					cerr << card->ID << " ";
				}
				cerr << endl;
			}

			// FIGHT
			// Go face, play what you can
			// hit guard if present
			string action = "";


			// SUMMON FIRST
			// first priority is a card that uses all of my mana
			for (auto& card : hero.handCards)
			{
				if (card.cost == hero.mana)
				{
					if (card.type == GREEN && !hero.boardCards.empty())
					{
						action.append("USE ");
						action.append(to_string(card.ID));
						action.append(" ");
						action.append(to_string(hero.boardCards.front().ID));
						action.append(";");
						hero.mana -= card.cost;
					}

					if (card.type == RED && !enemy.boardCards.empty())
					{
						action.append("USE ");
						action.append(to_string(card.ID));
						action.append(" ");
						action.append(to_string(enemy.boardCards.front().ID));
						action.append(";");
						hero.mana -= card.cost;
					}

					else if (card.type == BLUE)
					{
						action.append("USE ");
						action.append(to_string(card.ID));
						action.append(";");
						hero.mana -= card.cost;
					}

					else if (card.type == MINION)
					{
						action.append("SUMMON ");
						action.append(to_string(card.ID));
						action.append(";");
						hero.mana -= card.cost;
						hero.boardCards.push_back(card);
					}
				}
			}


			for (auto& card : hero.handCards)
			{
				if (card.type != MINION) { continue; }
				if (card.cost <= hero.mana)
				{
					action.append("SUMMON ");
					action.append(to_string(card.ID));
					action.append(";");
					hero.mana -= card.cost;
					hero.boardCards.push_back(card);
				}
			}

			// USE ITEMS
			for (auto& card : hero.handCards)
			{
				if (card.type == MINION) { continue; }
				if (card.cost <= hero.mana)
				{
					if (card.type == GREEN && !hero.boardCards.empty())
					{
						action.append("USE ");
						action.append(to_string(card.ID));
						action.append(" ");
						action.append(to_string(hero.boardCards.front().ID));
						action.append(";");
						hero.mana -= card.cost;
						hero.boardCards.front().attack += card.attack;
						hero.boardCards.front().defense += card.defense;
					}

					if (card.type == RED && !enemy.boardCards.empty())
					{
						action.append("USE ");
						action.append(to_string(card.ID));
						action.append(" ");
						action.append(to_string(enemy.boardCards.front().ID));
						action.append(";");
						hero.mana -= card.cost;
						enemy.boardCards.front().attack -= card.attack;
						enemy.boardCards.front().defense -= card.defense;
					}

					else if (card.type == BLUE)
					{
						action.append("USE ");
						action.append(to_string(card.ID));
						action.append(";");
						hero.mana -= card.cost;
					}
				}
			}

			// THEN ATTACK

			vector<Card*> enemyGuarders;
			enemyGuarders.clear();


			for (auto& enemyCard : enemy.boardCards)
			{
				if (enemyCard.guard && enemyCard.defense > 0)
				{
					enemyGuarders.push_back(&enemyCard);
				}
			}

			// Need a somewhat efficient way through guards
			// Try to hit for exact health if possible
			for (auto guarder : enemyGuarders)
			{
				if (guarder->ward) { continue; }

				for (auto& minion : hero.boardCards)
				{
					if (minion.attack == 0 || !minion.canAttack) { continue; }

					if (minion.attack == guarder->defense || minion.lethal)
					{
						action.append("ATTACK ");
						action.append(to_string(minion.ID));
						action.append(" ");
						action.append(to_string(guarder->ID));
						action.append(";");

						guarder->defense = 0;
						minion.canAttack = false;
					}
				}
			}

			for (auto& card : hero.boardCards)
			{
				if (card.attack == 0 || !card.canAttack) { continue; }
				int ID = -1;
				Card* guarder = nullptr;
				for (auto& enemyCard : enemy.boardCards)
				{
					if (enemyCard.guard && enemyCard.defense > 0)
					{
						ID = enemyCard.ID;
						guarder = &enemyCard;
					}
				}

				if (ID == -1)
				{
					// Make good trades if available
					int bestTrade = 0;
					Card* bestAttack = nullptr;
					for (auto& enemyCard : enemy.boardCards)
					{
						if (enemyCard.ward) { continue; }

						if (enemyCard.defense == card.attack)
						{
							int diff = abs(enemyCard.attack - card.defense);
							if (diff > bestTrade)
							{
								bestTrade = diff;
								bestAttack = &enemyCard;
							}
						}
					}

					if (bestTrade > 2 && bestAttack != nullptr)
					{
						ID = bestAttack->ID;
					}
				}

				action.append("ATTACK ");
				action.append(to_string(card.ID));
				action.append(" ");
				action.append(to_string(ID));
				action.append(";");

				if (guarder != nullptr)
				{
					guarder->defense -= card.attack;
				}
			}

			// TRY TO SUMMON AGAIN

			for (auto& card : hero.handCards)
			{
				if (card.cost <= hero.mana)
				{
					action.append("SUMMON ");
					action.append(to_string(card.ID));
					action.append(";");
					hero.mana -= card.cost;
					hero.boardCards.push_back(card);
				}
			}

			cout << action << endl;
		}




	}
}