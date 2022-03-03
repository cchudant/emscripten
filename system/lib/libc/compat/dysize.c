int dysize(int year) {
	int leap = ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)));
	return leap ? 366 : 365;
}
