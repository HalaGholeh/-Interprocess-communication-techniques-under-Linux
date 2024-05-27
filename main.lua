local incoms = {}
local customers ={}
local items ={}
local lasts = {}
local readTimer = 0
local readInterval = 0 -- 2 seconds
local childPipeRead = false -- Flag for readFromChildPipe execution
local numPipeDelay = false -- Flag to delay readFromNumPipe execution
local displayDelayTimer = 0
local displayDelayInterval = 5 -- 5 seconds delay before hiding squares
-- Declare startY at the top of your script to ensure global scope
local startY
local additionalRowStartY
local squareSize = 100  -- Also declare squareSize globally if it's used in love.draw()
local gap = 20          -- Same for gap

local numPipePath = "/tmp/num"
        local fileNum, errNum
        fileNum, errNum = io.open(numPipePath, "r")
            if errNum and not errNum:find("Interrupted system call") then
                print("Error opening num pipe:", errNum)  -- Debugging message
                --break
            end
        -- Attempt to open the pipe, handling interrupted system calls
        --repeat

        --until fileNum

function love.load()
    --------------------------
    love.window.setTitle("Supermarket")
    love.graphics.setBackgroundColor(1, 1, 1)

    startY = 50
    local windowWidth = love.graphics.getWidth()
    local totalWidth = (5 * squareSize) + (4 * gap)
    local startX = (windowWidth - totalWidth) / 2

    -- Initialize the arrays
    incoms = {}
    customers = {}
    items = {}
    lasts={}
    additionalRowStartY = startY + squareSize + gap -- Start Y for the new rows
    -- Initialize squares for each row
    for i = 1, 5 do
        -- Incoms row
        incoms[i] = {
            number = i,
            x = startX + (i - 1) * (squareSize + gap),
            y = startY,
            size = squareSize,
            color = {1, 0, 0}, -- Red color for the incoms row
            visible = true,
            text =""
        }

        -- Customers row
        customers[i] = {
            number = i,
            x = startX + (i - 1) * (squareSize + gap),
            y = startY + squareSize + gap, -- Position below the incoms row
            size = squareSize,
            color = {0, 255, 255}, -- Blue color for the customers row
            visible = true,
            text=""
        }

        -- Items row
        items[i] = {
            number = i,
            x = startX + (i - 1) * (squareSize + gap),
            y = startY + 2 * (squareSize + gap), -- Position below the customers row
            size = squareSize,
            color = {1, 1, 0}, -- Yellow color for the items row
            visible = true,
            text=""
        }
    end
    
     lasts[1] = {
        number = 1,
        x = startX + (1 - 1) * (squareSize + gap),
        y = startY + 3 * (squareSize + gap),
        size = squareSize,
        color = {1, 0, 0}, -- Red color for the incoms row
        visible = true,
        text ="0"
    }
    lasts[2] = {
        number = 2,
        x = startX + (2 - 1) * (squareSize + gap),
        y = startY + 3 * (squareSize + gap),
        size = squareSize*4,
        color = {1, 0, 0}, -- Red color for the incoms row
        visible = true,
        text ="openning :>"
    }

    -- Initialize the timer
    readTimer = 0
end



function readFromChildPipe()
    if not childPipeRead then
        local childPipePath = "/tmp/child_pipe"
        local fileChild, errChild = io.open(childPipePath, "r")

        if fileChild then
            local message = fileChild:read("*line")
            fileChild:close()

            if message then
                -- Show all squares
                for i = 1, 5 do
                    incoms[i].visible = true
                end
                numPipeDelay = true -- Enable reading from num pipe after this
            end
        else
            print("Error opening child pipe: " .. tostring(errChild))
        end
        childPipeRead = true -- Set the flag to true after first execution
    end
end


function readFromNumPipe()
    if numPipeDelay then

        -- Read from the pipe if it's successfully opened
        if fileNum then
            local message = fileNum:read("*line")
            if message then
                -- Debugging message
                local num1, num2, num3, num4, num5 = string.match(message, "([%d%.]+)%s+([%d%.]+)%s+([%d%.]+)%s+([%d%.]+)%s+([%d%.]+)")
                num1,num2,num3,num4,num5 = tonumber(num1), tonumber(num2), tonumber(num3),tonumber(num4),tonumber(num5)
                print("Message read from num pipe:", message)
                if num1 == 0 then
                    if num2 and incoms[num2] then
                        -- Hide the specific square
                        incoms[num2].visible = false
                    end
                elseif num1 == 1 then
                    lasts[1].text = tostring(num2)
                elseif num1 == 2 then
                    -- Hide the specific square
                    incoms[num2].text = tostring(num4)
                    customers[num2].text = tostring(num3)
                    items[num2].text = tostring(num5)
                elseif num1 == 3 then
                    lasts[2].text = "we are closing, too many cashiers left"
                elseif num1 == 4 then
                    lasts[2].text = "we are closing, too many customers left"
                elseif num1 == 5 then
                    lasts[2].text = "we are closing, we hit the target"
                end
                
                
            end
            --fileNum:close()
        end
    end
end



function love.update(dt)
    readTimer = readTimer + dt
    -- Initial setup: Display all squares and start delay timer
    if readTimer >= readInterval then
        if not childPipeRead then
            readFromChildPipe()
            displayDelayTimer = displayDelayTimer + dt -- Start delay timer after first read
        elseif displayDelayTimer >= displayDelayInterval then
            readFromNumPipe() -- Start reading from num pipe after delay
        end
        readTimer = 0 -- Reset the readTimer
    end

    -- Update display delay timer
    if childPipeRead and displayDelayTimer < displayDelayInterval then
        displayDelayTimer = displayDelayTimer + dt
    end
end


function love.draw()
    love.graphics.clear()
    -- Set text color to black
    love.graphics.setColor(1, 1, 1)

    -- Draw the text for each row
    love.graphics.print("Incomes", 10, startY - 20)
    love.graphics.print("Customers", 10, startY + squareSize + gap - 20)
    love.graphics.print("Items", 10, startY + 2 * (squareSize + gap) - 20)
    love.graphics.print("leaving Customers", 10, startY + 3 * (squareSize + gap) - 20)
    local function drawSquares2(array, row)
        for i, square in ipairs(array) do
            if square.visible then
                -- Draw the square
                love.graphics.setColor(square.color)
                if i==2 then
                    love.graphics.rectangle("fill", square.x, square.y, square.size+(3*gap), square.size/4)
                    local font = love.graphics.newFont(20, "normal")
                    love.graphics.setFont(font)
                    -- Draw the number on the square
                    love.graphics.setColor(0, 0, 0) -- Set text color to black
                    love.graphics.printf(tostring(square.text), square.x+10, square.y + square.size /8-6, square.size, "left")
                else
                love.graphics.rectangle("fill", square.x, square.y, square.size, square.size)
                local font = love.graphics.newFont(20, "normal")
                love.graphics.setFont(font)
                -- Draw the number on the square
                    love.graphics.setColor(0, 0, 0) -- Set text color to black
                    love.graphics.printf(tostring(square.text), square.x, square.y + square.size /2-6, square.size, "center")
                end

            end
        end
    end
    -- Function to draw squares from a given array
    local function drawSquares(array, row)
        for i, square in ipairs(array) do
            if square.visible then
                -- Draw the square
                love.graphics.setColor(square.color)
                love.graphics.rectangle("fill", square.x, square.y, square.size, square.size)
                local font = love.graphics.newFont(20, "normal")
                love.graphics.setFont(font)
                -- Draw the number on the square
                love.graphics.setColor(0, 0, 0) -- Set text color to black
                love.graphics.printf(tostring(square.text), square.x, square.y + square.size / 2 - 6, square.size, "center")
            end
        end
    end
    


    -- Draw the squares for each array
    drawSquares(incoms, 1)      -- Draw squares for the first row
    drawSquares(customers, 2)   -- Draw squares for the second row
    drawSquares(items, 3)       -- Draw squares for the third row
    drawSquares2(lasts, 4)
end

function love.quit()
        fileNum:close()
end