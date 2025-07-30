//------------------------------------------------------------------------------------------------
//! Utility class for finding random positions in the world
//! Useful for spawning entities, placing objectives, or AI waypoints
class RandomPositionHelper
{
    //! Find a random position within a radius around a center point
    //! \param centerPos Center position to search around
    //! \param minRadius Minimum distance from center
    //! \param maxRadius Maximum distance from center
    //! \param validateTerrain If true, ensures position is on valid terrain
    //! \param maxAttempts Maximum number of attempts before giving up
    //! \return Valid world position or vector.Zero if none found
    static vector FindRandomPositionAroundPoint(vector centerPos, float minRadius, float maxRadius, bool validateTerrain = true, int maxAttempts = 50)
    {
        World world = GetGame().GetWorld();
        if (!world)
            return vector.Zero;
            
        for (int i = 0; i < maxAttempts; i++)
        {
            // Generate random angle and distance
            float angle = Math.RandomFloat(0, Math.PI2);
            float distance = Math.RandomFloat(minRadius, maxRadius);
            
            // Calculate position
            vector randomPos = centerPos;
            randomPos[0] = randomPos[0] + Math.Cos(angle) * distance;
            randomPos[2] = randomPos[2] + Math.Sin(angle) * distance;
            
            if (validateTerrain)
            {
                // Get terrain height at this position
                float terrainY = world.GetSurfaceY(randomPos[0], randomPos[2]);
                randomPos[1] = terrainY;
                
                // Validate the position
                if (IsValidTerrainPosition(randomPos))
                    return randomPos;
            }
            else
            {
                // Just use the terrain height without validation
                randomPos[1] = world.GetSurfaceY(randomPos[0], randomPos[2]);
                return randomPos;
            }
        }
        
        Print("RandomPositionHelper: Failed to find valid position after " + maxAttempts + " attempts", LogLevel.WARNING);
        return vector.Zero;
    }
    
    //! Find a random position within a rectangular area
    //! \param minBounds Minimum X,Z coordinates of the rectangle
    //! \param maxBounds Maximum X,Z coordinates of the rectangle
    //! \param validateTerrain If true, ensures position is on valid terrain
    //! \param maxAttempts Maximum number of attempts before giving up
    //! \return Valid world position or vector.Zero if none found
    static vector FindRandomPositionInRectangle(vector minBounds, vector maxBounds, bool validateTerrain = true, int maxAttempts = 50)
    {
        World world = GetGame().GetWorld();
        if (!world)
            return vector.Zero;
            
        for (int i = 0; i < maxAttempts; i++)
        {
            vector randomPos;
            randomPos[0] = Math.RandomFloat(minBounds[0], maxBounds[0]);
            randomPos[2] = Math.RandomFloat(minBounds[2], maxBounds[2]);
            
            // Get terrain height
            float terrainY = world.GetSurfaceY(randomPos[0], randomPos[2]);
            randomPos[1] = terrainY;
            
            if (!validateTerrain || IsValidTerrainPosition(randomPos))
                return randomPos;
        }
        
        Print("RandomPositionHelper: Failed to find valid rectangle position after " + maxAttempts + " attempts", LogLevel.WARNING);
        return vector.Zero;
    }
    
    //! Find a random position along a road network
    //! \param searchCenter Center point for road search
    //! \param searchRadius Radius to search for roads
    //! \param minDistanceFromRoad Minimum distance from road edge (negative = on road)
    //! \param maxDistanceFromRoad Maximum distance from road edge
    //! \param maxAttempts Maximum number of attempts
    //! \return Valid position near road or vector.Zero if none found
    static vector FindRandomRoadPosition(vector searchCenter, float searchRadius, float minDistanceFromRoad = -2.0, float maxDistanceFromRoad = 5.0, int maxAttempts = 30)
    {
        World world = GetGame().GetWorld();
        if (!world)
            return vector.Zero;
            
        // First find a random position in the search area
        for (int i = 0; i < maxAttempts; i++)
        {
            vector testPos = FindRandomPositionAroundPoint(searchCenter, 0, searchRadius, false, 5);
            if (testPos == vector.Zero)
                continue;
                
            // Check if there's a road nearby
            vector roadPos;
            if (FindNearestRoad(testPos, 50.0, roadPos))
            {
                // Generate position relative to the road
                float roadDistance = Math.RandomFloat(minDistanceFromRoad, maxDistanceFromRoad);
                vector dirToRoad = (testPos - roadPos).Normalized();
                vector finalPos = roadPos + (dirToRoad * roadDistance);
                
                finalPos[1] = world.GetSurfaceY(finalPos[0], finalPos[2]);
                
                if (IsValidTerrainPosition(finalPos))
                    return finalPos;
            }
        }
        
        Print("RandomPositionHelper: Failed to find valid road position after " + maxAttempts + " attempts", LogLevel.WARNING);
        return vector.Zero;
    }
    
    //! Find a random position away from players
    //! \param minDistanceFromPlayers Minimum distance from any player
    //! \param searchCenter Center of search area
    //! \param searchRadius Radius of search area
    //! \param maxAttempts Maximum attempts to find position
    //! \return Valid position or vector.Zero if none found
    static vector FindRandomPositionAwayFromPlayers(float minDistanceFromPlayers, vector searchCenter, float searchRadius, int maxAttempts = 50)
    {
        array<int> playerIds = {};
        GetGame().GetPlayerManager().GetPlayers(playerIds);
        
        for (int i = 0; i < maxAttempts; i++)
        {
            vector testPos = FindRandomPositionAroundPoint(searchCenter, 0, searchRadius, true, 5);
            if (testPos == vector.Zero)
                continue;
                
            bool tooCloseToPlayer = false;
            
            // Check distance to all players
            foreach (int playerId : playerIds)
            {
                IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
                if (!playerEntity)
                    continue;
                    
                vector playerPos = playerEntity.GetOrigin();
                float distanceToPlayer = vector.Distance(testPos, playerPos);
                
                if (distanceToPlayer < minDistanceFromPlayers)
                {
                    tooCloseToPlayer = true;
                    break;
                }
            }
            
            if (!tooCloseToPlayer)
                return testPos;
        }
        
        Print("RandomPositionHelper: Failed to find position away from players after " + maxAttempts + " attempts", LogLevel.WARNING);
        return vector.Zero;
    }
    
    //! Validate if a terrain position is suitable for spawning
    //! \param pos Position to validate
    //! \return True if position is valid
    protected static bool IsValidTerrainPosition(vector pos)
    {
        World world = GetGame().GetWorld();
        if (!world)
            return false;
            
        // Check if position is in water
        float waterLevel = world.GetOceanBaseHeight();
        if (pos[1] <= waterLevel + 0.5) // 0.5m above water minimum
            return false;
            
        // Check terrain slope (too steep = invalid)
        float slopeThreshold = 0.7; // Roughly 35 degrees
        vector normal = world.GetSurfaceNormal(pos[0], pos[2]);
        if (normal[1] < slopeThreshold)
            return false;
            
        // Additional checks can be added here:
        // - Check for buildings/obstacles
        // - Check for restricted areas
        // - Check surface material type
        
        return true;
    }
    
    //! Find the nearest road to a position (simplified implementation)
    //! \param pos Position to search from
    //! \param maxSearchDistance Maximum search distance
    //! \param[out] roadPos Found road position
    //! \return True if road found
    protected static bool FindNearestRoad(vector pos, float maxSearchDistance, out vector roadPos)
    {
        // This is a simplified implementation
        // In a real scenario, you'd query the road network or use pathfinding
        World world = GetGame().GetWorld();
        if (!world)
            return false;
            
        // Sample points in a grid around the position to find road-like terrain
        float stepSize = 5.0;
        int steps = Math.Floor(maxSearchDistance / stepSize);
        
        for (int x = -steps; x <= steps; x++)
        {
            for (int z = -steps; z <= steps; z++)
            {
                vector testPos = pos;
                testPos[0] += x * stepSize;
                testPos[2] += z * stepSize;
                testPos[1] = world.GetSurfaceY(testPos[0], testPos[2]);
                
                // Check if this looks like a road (flat, level terrain)
                vector normal = world.GetSurfaceNormal(testPos[0], testPos[2]);
                if (normal[1] > 0.95) // Very flat surface
                {
                    roadPos = testPos;
                    return true;
                }
            }
        }
        
        return false;
    }
}

//------------------------------------------------------------------------------------------------
//! Example usage function - can be called from your scenario scripts
void TestRandomPositions()
{
    // Example: Find random position around a center point
    vector centerPos = "5000 0 5000"; // Adjust for your map
    vector randomPos1 = RandomPositionHelper.FindRandomPositionAroundPoint(centerPos, 100, 500, true);
    
    if (randomPos1 != vector.Zero)
        Print("Found random position: " + randomPos1.ToString());
    
    // Example: Find position in rectangular area
    vector minBounds = "4000 0 4000";
    vector maxBounds = "6000 0 6000";
    vector randomPos2 = RandomPositionHelper.FindRandomPositionInRectangle(minBounds, maxBounds, true);
    
    if (randomPos2 != vector.Zero)
        Print("Found rectangle position: " + randomPos2.ToString());
    
    // Example: Find position away from players
    vector searchCenter = "5000 0 5000";
    vector randomPos3 = RandomPositionHelper.FindRandomPositionAwayFromPlayers(200, searchCenter, 1000);
    
    if (randomPos3 != vector.Zero)
        Print("Found position away from players: " + randomPos3.ToString());
}