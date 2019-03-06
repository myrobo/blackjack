#include <iostream>
#include <vector>
#include <functional>
#include <random>

class Player
{/*{{{*/
	public:
		enum Action { ACTION_STAND=0, ACTION_HIT=1, ACTION_DOUBLE=2, ACTION_SPLIT=3, ACTION_SURRENDER=4 };

		Player(){};
		Player(
				std::string name,
				std::function< double(std::vector<int>&) > bet_function,
				std::function< bool(std::vector<int>&)> insurance_function,
				std::function< Action(std::vector<int>&,std::vector<int>&,int,bool)> action_function );
		std::string name_;
		std::function< double(std::vector<int>&) > bet_function_;
		std::function< bool(std::vector<int>&) > insurance_function_;
		std::function< Action(std::vector<int>&,std::vector<int>&,int,bool) > action_function_;
		int split_base_ = -1;
		bool is_insurance_ = false;
		bool is_surrender_ = false;
		void draw( int card );
		void reset();
		double bet_ = 0.0;
		double coin_ = 0.0;
		int n_buttles_ = 0;
		std::vector<int> hand_;
	private:
};/*}}}*/

Player::Player(
		std::string name,
		std::function< double(std::vector<int>&) > bet_function,
		std::function< bool(std::vector<int>&) > insurance_function,
		std::function< Action(std::vector<int>&,std::vector<int>&,int,bool) > action_function )
{/*{{{*/
	name_ = name;
	bet_function_ = bet_function;
	insurance_function_ = insurance_function;
	action_function_ = action_function;
}/*}}}*/

void Player::reset()
{/*{{{*/
	is_insurance_ = false;
	is_surrender_ = false;
	hand_.clear();
	++n_buttles_;
}/*}}}*/

void Player::draw( int card )
{/*{{{*/
	hand_.push_back( card );
}/*}}}*/

class Dealer
{/*{{{*/
	public:
		Dealer( std::vector<Player>& players, bool is_surrender_allowed = true, int n_buttles = 1000 );
		void execute();
		void show_result();
	private:
		int deal();
		bool is_BJ( const std::vector<int>& );
		void check_shuffle();
		void generate_deck();
		int check_hand( std::vector<int>& hand, bool verbose = false, std::string = "" );
		double calc_payout( Player& player );
		std::vector<int> deck_;
		std::vector<Player> players_;
		std::vector<int> hand_;
		int n_buttles_;
		bool is_surrender_allowed_;
		int shuffle_timing_;
		int shuffle_timing_min_ = 40;
		int shuffle_timing_max_ = 60;
		int num_of_sets_= 6;
		int n_players_;
		std::mt19937 mt_;
		std::uniform_int_distribution<> rand_shuffle_, rand_shuffle_timing_;
};/*}}}*/

Dealer::Dealer( std::vector<Player>& players, bool is_surrender_allowed, int n_buttles )
{/*{{{*/
	is_surrender_allowed_ = is_surrender_allowed;
	n_buttles_ = n_buttles;
	players_ = players;
	n_players_ = players_.size();
	for ( int i=0; i<players_.size(); ++i ) {
		players_[i].split_base_ = i;
	}
	std::random_device seed;
	mt_ = std::mt19937( seed() );
	rand_shuffle_ = std::uniform_int_distribution<>( 1, 52 * num_of_sets_ );
	rand_shuffle_timing_ = std::uniform_int_distribution<>( shuffle_timing_min_, shuffle_timing_max_ );
	shuffle_timing_ = rand_shuffle_timing_( mt_ );
	std::cout<< "shuffle_timing : " << shuffle_timing_ <<std::endl;
}/*}}}*/

void Dealer::execute()
{/*{{{*/
	while ( 0 < n_buttles_ ) {
		std::cout<<std::endl<<std::endl<<std::endl;
		std::cout<< "##### new game #####" <<std::endl;
		--n_buttles_;
		check_shuffle();
		hand_.clear();
		hand_.push_back( deal() );

		for ( auto& player : players_ ) {   // deal first card
			player.bet_ = player.bet_function_( deck_ );
			player.draw( deal() );
			player.draw( deal() );
			if ( hand_[0] == 1 && !is_BJ( player.hand_ ) ) {
				player.is_insurance_ = player.insurance_function_( deck_ );
			}
		}

		for ( int i=0; i<players_.size(); ++i ) {

			while (1) {
				int player_sum = check_hand( players_[i].hand_, true, players_[i].name_ );

				if ( 21 <= player_sum ) break;   // BJ or BUST
				else {   // not BJ or BUST

					Player::Action action = players_[i].action_function_( deck_, players_[i].hand_, hand_[0], is_surrender_allowed_ );

					if ( action == Player::ACTION_SPLIT ) {   // split
						std::cout<< "Action : [SPLIT]" <<std::endl;
						players_[i].hand_.pop_back();
						Player split_player = players_[i];
						split_player.name_ += "-split";
						split_player.is_insurance_ = false;
						split_player.draw( deal() );
						players_.push_back( split_player );
						// players_.insert( players_.begin()+i+1, split_player );
					}

					else if ( action == Player::ACTION_DOUBLE ) {   // double
						std::cout<< "Action : [DOUBLE]" <<std::endl;
						players_[i].bet_ *= 2;
						players_[i].draw( deal() );
						check_hand( players_[i].hand_, true, players_[i].name_ );
						break;
					}

					else if ( action == Player::ACTION_SURRENDER ) {
						std::cout<< "Action : [SURRENDER]" <<std::endl;
						players_[i].is_surrender_ = true;
						break;
					}
					else if ( action == Player::ACTION_STAND ) {
						std::cout<< "Action : [STAND]" <<std::endl;
						break;
					}

					std::cout<< "Action : [HIT]" <<std::endl;
					players_[i].draw( deal() );

					// std::string test;
					// std::cin >> test;
				}
			}
					// std::string test;
					// std::cin >> test;
		}

		// dealer draw ( dealer must to draw 16 and stand on all 17s )
		do {
			hand_.push_back( deal() );
		} while ( check_hand( hand_ ) < 17 );
		check_hand( hand_, true, "Dealer" );
		
		// pay reward
		for ( auto& player : players_ ) {
			players_[player.split_base_].coin_ += calc_payout( player );
			std::cout<< "coin : " << player.coin_ <<std::endl;
		}

		players_.resize( n_players_ );   // remove split players
		for ( auto& player : players_ ) {
			player.reset();
		}
	}

}/*}}}*/

int Dealer::deal()
{/*{{{*/
	int card = deck_.back();
	deck_.pop_back();
	return card;
}/*}}}*/

bool Dealer::is_BJ( const std::vector<int>& hand )
{/*{{{*/
	if ( hand.size() == 2 ) {
		if ( hand[0] == 1 && hand[1] == 10 ) return true;
		if ( hand[1] == 1 && hand[0] == 10 ) return true;
	}
	return false;
}/*}}}*/

void Dealer::check_shuffle()
{/*{{{*/
	if ( deck_.size() <= shuffle_timing_ ) {
		std::cout<< "===== shuffled =====" <<std::endl;
		generate_deck();
		shuffle_timing_ = rand_shuffle_timing_( mt_ );
	}
}/*}}}*/

void Dealer::generate_deck()
{/*{{{*/
	if ( num_of_sets_ <= 0 ) {
		std::cout << "Error. num_of_sets must be positive." << std::endl;
		exit(1);
	}

	// generate
	deck_.clear();
	for ( int i=0; i<num_of_sets_; ++i ) {
		for ( int c=0; c<52; ++c ) {
			int card_num = c%13 + 1;   // generate 1 to 13
			if ( 11 <= card_num ) card_num = 10;   // reguard 11, 12, 13 as 10
			// std::cout<< card_num << " ";
			deck_.push_back( card_num );
		}
	}

	// shuffle
	for ( int i=0; i<deck_.size(); ++i ) {
		int swap = deck_[i];
		int swap_n = rand_shuffle_( mt_ );
		deck_[i] = deck_[swap_n];
		deck_[swap_n] = swap;
	}
}/*}}}*/

int Dealer::check_hand( std::vector<int>& hand, bool verbose, std::string name )
{/*{{{*/
	int sum = 0;
	bool has_ace = false;
	if ( verbose ) std::cout<< name << "'s hand is ... [ ";
	for ( int card : hand ) {
		sum += card;
		if ( verbose ) std::cout<< card << " ";
		if ( card == 1 ) has_ace = true;
		// std::cout<< sum << "###" <<std::endl;
	}

	if ( has_ace && hand.size() == 2 && sum == 11 ) {
		if ( verbose ) std::cout<< "]   Sum : [BJ!]" << std::endl;
		return 21;
	}
	if ( verbose ) std::cout<< "]   Sum : " << sum << " ";
	if ( 21 < sum ) {
		if ( verbose ) std::cout<< "[BUST]" << std::endl;
	}
	else if ( sum < 11 && has_ace ) {
		if ( verbose ) std::cout<< "or " << sum + 10 << std::endl;
		return sum + 10;
	}

	if ( verbose ) std::cout<<std::endl;
	return sum;
}/*}}}*/

double Dealer::calc_payout( Player& player )
{/*{{{*/
	int player_status = check_hand( player.hand_ );
	int dealer_status = check_hand( hand_ );

	std::cout<<std::endl;
	std::cout<< "--- Result ---" <<std::endl;
	std::cout<< player.name_ << " (" << player_status << ") vs (" << dealer_status << ") Dealer   ";

	double payout = 0.0;

	// player surrender
	if ( player.is_surrender_ ) {
			std::cout<< "[Surrender]" <<std::endl;
			payout -= player.bet_ / 2.0;
	}

	// player bust
	else if ( 21 < player_status ) {
			std::cout<< "[Lose]" <<std::endl;
			payout -= player.bet_;
	}

	// player BJ
	else if ( is_BJ( player.hand_ ) ) {
		if ( is_BJ( hand_ ) ) {
			std::cout<< "[Even]" <<std::endl;
		}
		else {
			std::cout<< "[Win (BJ)]" <<std::endl;
			payout += player.bet_ * 1.5;
		}
	}

	// player not bust
	else {
		if ( 21 < dealer_status ) {
			std::cout<< "[Win]" <<std::endl;
			payout += player.bet_;
		}
		else if ( is_BJ( hand_ ) ) {
			std::cout<< "[Lose]" <<std::endl;
			payout += - player.bet_;
		}
		else if ( dealer_status == player_status ) {
			std::cout<< "[Even]" <<std::endl;
		}
		else if ( dealer_status < player_status ) {
			std::cout<< "[Win]" <<std::endl;
			payout += player.bet_;
		}
		else {
			std::cout<< "[Lose]" <<std::endl;
			payout += - player.bet_;
		}
	}

	if ( player.is_insurance_ && hand_[0] == 1 && !player.is_surrender_ ) {
		if ( hand_[1] == 10 ) {
			std::cout<< "Insurance : [Win]" <<std::endl;
			payout += player.bet_;
		}
		else {
			std::cout<< "Insurance : [Lose]" <<std::endl;
			payout -= player.bet_;
		}
	}

	std::cout<< "Payout : " << payout <<std::endl;
	return payout;
}/*}}}*/

void Dealer::show_result()
{/*{{{*/
	for ( const auto& player : players_ ) {
		std::cout<<std::endl;
		std::cout<< "=====" << player.name_ << "'s Result =====" <<std::endl;
		std::cout<< "Games : " << player.n_buttles_ <<std::endl;
		std::cout<< "Coin : " << player.coin_ <<std::endl;
		std::cout<< "Reduction rate per game : " << ( 1.0 + player.coin_ / player.n_buttles_ ) * 100.0 << "%" <<std::endl;
	}
}/*}}}*/

double bet_strategy( std::vector<int>& deck )
{/*{{{*/
	return 1.0;
}/*}}}*/

bool insurance_strategy( std::vector<int>& deck )
{/*{{{*/
	return false;
}/*}}}*/

Player::Action manual_strategy( std::vector<int>& deck, std::vector<int>& hand, int upcard, bool is_surrender_allowed )
{/*{{{*/
	std::cout<< "Dealer's upcard is ... " << upcard <<std::endl;
	bool is_split_ok = false, is_surrender_ok = false;

	std::string choices_string = "0:STAND 1:HIT";
	if ( hand.size() == 2 ) {
		choices_string += " 2:DOUBLE";
		if ( hand[0] == hand[1] ) {
			choices_string += " 3:SPLIT";
			is_split_ok = true;
		}
		if ( is_surrender_allowed ) {
			choices_string += " 4:SURRENDER";
			is_surrender_ok = true;
		}
	}
	std::cout<< choices_string <<std::endl;

	bool is_ok = false;
	while (1) {
		std::string input;
		std::cin >> input;
		if ( input == "0" ) {
			is_ok = true;
			return Player::ACTION_STAND;
		}
		if ( input == "1" ) {
			is_ok = true;
			return Player::ACTION_HIT;
		}
		if ( input == "2" && hand.size() == 2 ) {
			is_ok = true;
			return Player::ACTION_DOUBLE;
		}
		if ( input == "3" && is_split_ok ) {
			is_ok = true;
			return Player::ACTION_SPLIT;
		}
		if ( input == "4" && is_surrender_ok ) {
			is_ok = true;
			return Player::ACTION_SURRENDER;
		}
	}

}/*}}}*/

Player::Action basic_strategy( std::vector<int>& deck, std::vector<int>& hand, int upcard, bool is_surrender_allowed )
{/*{{{*/
	std::cout<< "Dealer's upcard is ... " << upcard <<std::endl;

	int sum = 0;
	bool has_ace = false;
	for ( auto card : hand ) {
		sum += card;
		if ( card == 1 ) has_ace = true;
	}

	// split
	if ( hand.size() == 2 && hand[0] == hand[1] ) {
		if ( hand[0] == 8 || hand[0] == 1 ) {
			return Player::ACTION_SPLIT;
		}
		else if ( hand[0] == 2 || hand[0] == 3 || hand[0] == 7 ) {
			if ( 2 <= upcard && upcard <= 7 ) return Player::ACTION_SPLIT;
			else return Player::ACTION_HIT;
		}
		else if ( hand[0] == 6 ) {
			if ( 2 <= upcard && upcard <=6 ) return Player::ACTION_SPLIT;
			else return Player::ACTION_HIT;
		}
		else if ( hand[0] == 4 ) {
			if ( upcard <= 5 && upcard <= 6 ) return Player::ACTION_SPLIT;
			else return Player::ACTION_HIT;
		}
		else if ( hand[0] == 10 ) {
			return Player::ACTION_STAND;
		}
		else if ( hand[0] == 9 ) {
			if ( upcard == 7 || upcard == 10 || upcard == 1 ) return Player::ACTION_STAND;
			else return Player::ACTION_SPLIT;
		}
		else if ( hand[0] == 5 ) {
			if ( upcard == 10 || upcard == 1 ) return Player::ACTION_HIT;
			else return Player::ACTION_DOUBLE;
		}
	}

	// soft hand
	else if ( sum <= 11 && has_ace ) {
		if ( sum == 9 || sum == 10 ) {   // A & 8,9
			return Player::ACTION_STAND;
		}
		else if ( sum == 8 ) {   // A & 7
			if ( hand.size() == 2 && 3 <= upcard && upcard <= 6 ) return Player::ACTION_DOUBLE;
			// else if ( upcard == 2 || upcard == 7 || upcard == 8 ) return Player::ACTION_STAND;
			// if ( 2 <= upcard && upcard <= 8 ) return Player::ACTION_STAND;
			else if ( 2 <= upcard && upcard <= 8 ) return Player::ACTION_STAND;
			else return Player::ACTION_HIT;
		}
		else if ( sum == 7 ) {   // A & 6
			if ( hand.size() == 2 && 3 <= upcard && upcard <= 6 ) return Player::ACTION_DOUBLE;
			else return Player::ACTION_HIT;
		}
		else if ( sum == 5 || sum == 6 ) {   // A & 4,5
			if ( hand.size() == 2 && 4 <= upcard && upcard <= 6 ) return Player::ACTION_DOUBLE;
			else return Player::ACTION_HIT;
		}
		else if ( sum == 3 || sum == 4 ) {   // A & 2,3
			if ( hand.size() == 2 && 5 <= upcard && upcard <= 6 ) return Player::ACTION_DOUBLE;
			else return Player::ACTION_HIT;
		}
	}

	// hard hand
	else {
		if ( sum <= 8 ) {
			return Player::ACTION_HIT;
		}
		else if ( sum == 9 ) {
			if ( hand.size() == 2 && 3 <= upcard && upcard <= 6 ) return Player::ACTION_DOUBLE;
			return Player::ACTION_HIT;
		}
		else if ( sum == 10 ) {
			if ( hand.size() == 2 && 2 <= upcard && upcard <= 9 ) return Player::ACTION_DOUBLE;
			return Player::ACTION_HIT;
		}
		else if ( sum == 11 ) {
			if ( hand.size() == 2 && 2 <= upcard && upcard <= 10 ) return Player::ACTION_DOUBLE;
			return Player::ACTION_HIT;
		}
		else if ( sum == 12 ) {
			if ( 4 <= upcard && upcard <= 6 ) return Player::ACTION_STAND;
			return Player::ACTION_HIT;
		}
		else if ( 12 <= sum && sum <= 16 ) {
			if ( 2 <= upcard && upcard <= 6 ) return Player::ACTION_STAND;

			else if ( is_surrender_allowed && hand.size() == 2 ) {
				if ( sum == 15 && upcard == 10 ) return Player::ACTION_SURRENDER;
				else if ( sum == 16 && ( upcard == 9 || upcard == 10 || upcard == 1 ) ) return Player::ACTION_SURRENDER;
			}

			return Player::ACTION_HIT;
		}
		else {
			return Player::ACTION_STAND;
		}
	}

	return  Player::ACTION_STAND;
}/*}}}*/

int main()
{
	int buttles = 100000;
	bool can_surrender = true;

	// Player player_manual( std::string("player_manual"), bet_strategy, insurance_strategy, manual_strategy );
	// std::vector<Player> player_list{ player_manual };
	
	Player player1( std::string("player1"), bet_strategy, insurance_strategy, basic_strategy );
	Player player2( std::string("player2"), bet_strategy, insurance_strategy, basic_strategy );
	Player player3( std::string("player3"), bet_strategy, insurance_strategy, basic_strategy );
	Player player4( std::string("player4"), bet_strategy, insurance_strategy, basic_strategy );
	Player player5( std::string("player5"), bet_strategy, insurance_strategy, basic_strategy );
	std::vector<Player> player_list{ player1, player2, player3, player4, player5 };

	Dealer dealer( player_list, can_surrender, buttles );
	dealer.execute();
	dealer.show_result();

	return 0;
}
