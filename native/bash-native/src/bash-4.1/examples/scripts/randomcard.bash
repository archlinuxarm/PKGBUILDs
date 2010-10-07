# The following prints a random card from a card deck.
#
# cribbed from the ksh93 book, example from page 70
#
# chet@po.cwru.edu
#
declare -i i=0

# load the deck
for suit in clubs diamonds hearts spades; do
	for n in ace 2 3 4 5 6 7 8 9 10 jack queen king; do
		card[i]="$n of $suit"
        	i=i+1		# let is not required with integer variables
	done
done

# and print a random card
echo ${card[RANDOM%52]}
