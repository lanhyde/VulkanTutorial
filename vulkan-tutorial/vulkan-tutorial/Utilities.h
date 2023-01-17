#pragma once

// Indices (locations) of queue families (if they exist)
struct QueueFamilyIndices
{
	int graphicsFamily = -1;		// Location of Graphics Queue Family

	// Check if queue families are valid
	bool isValid()
	{
		return graphicsFamily >= 0;
	}
};